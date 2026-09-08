#include "RendererDX11.h"

#include <cstring>
#include <vector>

#include <d3dcompiler.h>

#include "TextRenderer.h"

#pragma comment(lib, "d3dcompiler.lib")

namespace app {

namespace {
// Flat-color 2D pipeline. Positions are supplied already in NDC (x,y in
// [-1,1]); drawRect()/endFrame() do the pixel->NDC conversion on the CPU
// each frame since screen size changes with window resize.
const char* kShaderSource = R"(
struct VSInput {
    float2 pos : POSITION;
    float4 color : COLOR;
};
struct PSInput {
    float4 pos : SV_POSITION;
    float4 color : COLOR;
};
PSInput VSMain(VSInput input) {
    PSInput output;
    output.pos = float4(input.pos, 0.0, 1.0);
    output.color = input.color;
    return output;
}
float4 PSMain(PSInput input) : SV_TARGET {
    return input.color;
}
)";

// Textured 2D pipeline for glyph quads. The atlas alpha channel carries
// glyph coverage (see createFontAtlas()); RGB comes from the per-vertex
// color so the same atlas serves any text color.
const char* kTextShaderSource = R"(
struct VSInput {
    float2 pos : POSITION;
    float2 uv : TEXCOORD0;
    float4 color : COLOR;
};
struct PSInput {
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD0;
    float4 color : COLOR;
};
Texture2D gAtlas : register(t0);
SamplerState gSampler : register(s0);
PSInput VSMain(VSInput input) {
    PSInput output;
    output.pos = float4(input.pos, 0.0, 1.0);
    output.uv = input.uv;
    output.color = input.color;
    return output;
}
float4 PSMain(PSInput input) : SV_TARGET {
    float coverage = gAtlas.Sample(gSampler, input.uv).a;
    return float4(input.color.rgb, input.color.a * coverage);
}
)";
}  // namespace

bool RendererDX11::initialize(HWND hwnd, uint32_t width, uint32_t height) {
    DXGI_SWAP_CHAIN_DESC scDesc{};
    scDesc.BufferCount = 2;
    scDesc.BufferDesc.Width = width;
    scDesc.BufferDesc.Height = height;
    scDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    scDesc.BufferDesc.RefreshRate.Numerator = 60;
    scDesc.BufferDesc.RefreshRate.Denominator = 1;
    scDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    scDesc.OutputWindow = hwnd;
    scDesc.SampleDesc.Count = 1;
    scDesc.SampleDesc.Quality = 0;
    scDesc.Windowed = TRUE;
    scDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    UINT createFlags = 0;
#ifdef _DEBUG
    createFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    D3D_FEATURE_LEVEL requestedLevels[] = {D3D_FEATURE_LEVEL_11_0};
    D3D_FEATURE_LEVEL obtainedLevel{};

    HRESULT hr = D3D11CreateDeviceAndSwapChain(
        nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, createFlags,
        requestedLevels, ARRAYSIZE(requestedLevels), D3D11_SDK_VERSION,
        &scDesc, swapChain_.GetAddressOf(), device_.GetAddressOf(),
        &obtainedLevel, context_.GetAddressOf());

    if (FAILED(hr)) {
        return false;
    }

    width_ = width;
    height_ = height;
    return createRenderTarget() && createPipeline() && createFontAtlas() && createTextPipeline();
}

void RendererDX11::shutdown() {
    pendingVertices_.clear();
    pendingTextVertices_.clear();
    textVertexBuffer_.Reset();
    textVertexBufferCapacity_ = 0;
    fontSamplerState_.Reset();
    fontAtlasSRV_.Reset();
    textInputLayout_.Reset();
    textPixelShader_.Reset();
    textVertexShader_.Reset();
    alphaBlendState_.Reset();
    vertexBuffer_.Reset();
    vertexBufferCapacity_ = 0;
    inputLayout_.Reset();
    pixelShader_.Reset();
    vertexShader_.Reset();
    releaseRenderTarget();
    context_.Reset();
    swapChain_.Reset();
    device_.Reset();
}

bool RendererDX11::createRenderTarget() {
    Microsoft::WRL::ComPtr<ID3D11Texture2D> backBuffer;
    HRESULT hr = swapChain_->GetBuffer(0, IID_PPV_ARGS(backBuffer.GetAddressOf()));
    if (FAILED(hr)) {
        return false;
    }

    hr = device_->CreateRenderTargetView(backBuffer.Get(), nullptr, renderTargetView_.GetAddressOf());
    return SUCCEEDED(hr);
}

void RendererDX11::releaseRenderTarget() {
    renderTargetView_.Reset();
}

bool RendererDX11::createPipeline() {
    Microsoft::WRL::ComPtr<ID3DBlob> vsBlob;
    Microsoft::WRL::ComPtr<ID3DBlob> psBlob;
    Microsoft::WRL::ComPtr<ID3DBlob> errorBlob;

    UINT compileFlags = 0;
#ifdef _DEBUG
    compileFlags |= D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

    HRESULT hr = D3DCompile(kShaderSource, strlen(kShaderSource), nullptr, nullptr, nullptr, "VSMain", "vs_4_0",
                             compileFlags, 0, vsBlob.GetAddressOf(), errorBlob.GetAddressOf());
    if (FAILED(hr)) {
        return false;
    }

    hr = D3DCompile(kShaderSource, strlen(kShaderSource), nullptr, nullptr, nullptr, "PSMain", "ps_4_0",
                     compileFlags, 0, psBlob.GetAddressOf(), errorBlob.GetAddressOf());
    if (FAILED(hr)) {
        return false;
    }

    hr = device_->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr,
                                      vertexShader_.GetAddressOf());
    if (FAILED(hr)) {
        return false;
    }

    hr = device_->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr,
                                     pixelShader_.GetAddressOf());
    if (FAILED(hr)) {
        return false;
    }

    D3D11_INPUT_ELEMENT_DESC layoutDesc[] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 8, D3D11_INPUT_PER_VERTEX_DATA, 0},
    };
    hr = device_->CreateInputLayout(layoutDesc, ARRAYSIZE(layoutDesc), vsBlob->GetBufferPointer(),
                                     vsBlob->GetBufferSize(), inputLayout_.GetAddressOf());
    if (FAILED(hr)) {
        return false;
    }

    // Standard alpha-over blending. Flat-color rects always use alpha=1,
    // so this is a no-op for them and only affects glyph edges.
    D3D11_BLEND_DESC blendDesc{};
    blendDesc.RenderTarget[0].BlendEnable = TRUE;
    blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
    blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
    blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
    blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
    blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
    hr = device_->CreateBlendState(&blendDesc, alphaBlendState_.GetAddressOf());
    return SUCCEEDED(hr);
}

// Generates a fixed-cell font atlas (printable ASCII 32-126, see
// TextRenderer.h for grid layout) using GDI, then uploads it as a DX11
// shader-resource texture. Antialiased (grayscale, not ClearType) so the
// alpha-only coverage extraction below stays color-neutral.
bool RendererDX11::createFontAtlas() {
    constexpr int kCellW = 16;
    constexpr int kCellH = 24;
    const int atlasW = kCellW * kFontAtlasCols;
    const int atlasH = kCellH * kFontAtlasRows;
    fontCharWidth_ = static_cast<float>(kCellW);
    fontCharHeight_ = static_cast<float>(kCellH);

    HDC screenDC = GetDC(nullptr);
    HDC memDC = CreateCompatibleDC(screenDC);

    BITMAPINFO bmi{};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = atlasW;
    bmi.bmiHeader.biHeight = -atlasH;  // top-down DIB
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    void* bits = nullptr;
    HBITMAP dib = CreateDIBSection(memDC, &bmi, DIB_RGB_COLORS, &bits, nullptr, 0);
    if (!dib || !bits) {
        DeleteDC(memDC);
        ReleaseDC(nullptr, screenDC);
        return false;
    }

    HGDIOBJ oldBmp = SelectObject(memDC, dib);

    RECT full{0, 0, atlasW, atlasH};
    FillRect(memDC, &full, static_cast<HBRUSH>(GetStockObject(BLACK_BRUSH)));

    LOGFONTW logFont{};
    logFont.lfHeight = -(kCellH - 6);
    logFont.lfWeight = FW_NORMAL;
    logFont.lfQuality = ANTIALIASED_QUALITY;  // grayscale AA, not ClearType (avoids color fringing)
    logFont.lfCharSet = ANSI_CHARSET;
    logFont.lfPitchAndFamily = FIXED_PITCH | FF_MODERN;
    const wchar_t* faceName = L"Consolas";
    for (int i = 0; faceName[i] != 0 && i < LF_FACESIZE - 1; ++i) {
        logFont.lfFaceName[i] = faceName[i];
    }

    HFONT font = CreateFontIndirectW(&logFont);
    HGDIOBJ oldFont = SelectObject(memDC, font);
    SetBkMode(memDC, TRANSPARENT);
    SetTextColor(memDC, RGB(255, 255, 255));

    for (int c = kFontFirstAsciiChar; c <= kFontLastAsciiChar; ++c) {
        const int idx = c - kFontFirstAsciiChar;
        const int col = idx % kFontAtlasCols;
        const int row = idx / kFontAtlasCols;
        wchar_t ch[2] = {static_cast<wchar_t>(c), 0};
        RECT cellRect{col * kCellW, row * kCellH, (col + 1) * kCellW, (row + 1) * kCellH};
        DrawTextW(memDC, ch, 1, &cellRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    }

    SelectObject(memDC, oldFont);
    DeleteObject(font);

    // GDI's 32bpp DIB is B,G,R,(unused) per pixel; text is grayscale
    // (R==G==B) so any channel is a valid coverage value. Output RGBA with
    // white RGB (the vertex color supplies the real color) and alpha =
    // that grayscale coverage.
    std::vector<uint8_t> rgba(static_cast<size_t>(atlasW) * atlasH * 4);
    const uint8_t* src = static_cast<const uint8_t*>(bits);
    for (int i = 0; i < atlasW * atlasH; ++i) {
        const uint8_t coverage = src[i * 4 + 0];
        rgba[i * 4 + 0] = 255;
        rgba[i * 4 + 1] = 255;
        rgba[i * 4 + 2] = 255;
        rgba[i * 4 + 3] = coverage;
    }

    SelectObject(memDC, oldBmp);
    DeleteObject(dib);
    DeleteDC(memDC);
    ReleaseDC(nullptr, screenDC);

    D3D11_TEXTURE2D_DESC texDesc{};
    texDesc.Width = static_cast<UINT>(atlasW);
    texDesc.Height = static_cast<UINT>(atlasH);
    texDesc.MipLevels = 1;
    texDesc.ArraySize = 1;
    texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    texDesc.SampleDesc.Count = 1;
    texDesc.Usage = D3D11_USAGE_IMMUTABLE;
    texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

    D3D11_SUBRESOURCE_DATA initData{};
    initData.pSysMem = rgba.data();
    initData.SysMemPitch = static_cast<UINT>(atlasW * 4);

    Microsoft::WRL::ComPtr<ID3D11Texture2D> texture;
    HRESULT hr = device_->CreateTexture2D(&texDesc, &initData, texture.GetAddressOf());
    if (FAILED(hr)) {
        return false;
    }

    hr = device_->CreateShaderResourceView(texture.Get(), nullptr, fontAtlasSRV_.GetAddressOf());
    if (FAILED(hr)) {
        return false;
    }

    D3D11_SAMPLER_DESC sampDesc{};
    sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
    sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
    sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
    sampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
    hr = device_->CreateSamplerState(&sampDesc, fontSamplerState_.GetAddressOf());
    return SUCCEEDED(hr);
}

bool RendererDX11::createTextPipeline() {
    Microsoft::WRL::ComPtr<ID3DBlob> vsBlob;
    Microsoft::WRL::ComPtr<ID3DBlob> psBlob;
    Microsoft::WRL::ComPtr<ID3DBlob> errorBlob;

    UINT compileFlags = 0;
#ifdef _DEBUG
    compileFlags |= D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

    HRESULT hr = D3DCompile(kTextShaderSource, strlen(kTextShaderSource), nullptr, nullptr, nullptr, "VSMain",
                             "vs_4_0", compileFlags, 0, vsBlob.GetAddressOf(), errorBlob.GetAddressOf());
    if (FAILED(hr)) {
        return false;
    }

    hr = D3DCompile(kTextShaderSource, strlen(kTextShaderSource), nullptr, nullptr, nullptr, "PSMain", "ps_4_0",
                     compileFlags, 0, psBlob.GetAddressOf(), errorBlob.GetAddressOf());
    if (FAILED(hr)) {
        return false;
    }

    hr = device_->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr,
                                      textVertexShader_.GetAddressOf());
    if (FAILED(hr)) {
        return false;
    }

    hr = device_->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr,
                                     textPixelShader_.GetAddressOf());
    if (FAILED(hr)) {
        return false;
    }

    D3D11_INPUT_ELEMENT_DESC layoutDesc[] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 8, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 16, D3D11_INPUT_PER_VERTEX_DATA, 0},
    };
    hr = device_->CreateInputLayout(layoutDesc, ARRAYSIZE(layoutDesc), vsBlob->GetBufferPointer(),
                                     vsBlob->GetBufferSize(), textInputLayout_.GetAddressOf());
    return SUCCEEDED(hr);
}

bool RendererDX11::ensureVertexBufferCapacity(size_t vertexCount) {
    if (vertexCount <= vertexBufferCapacity_) {
        return true;
    }

    // Grow generously so a typical UI frame (regions + 88 keys, ~600 quads)
    // needs no more than one reallocation.
    size_t newCapacity = vertexBufferCapacity_ == 0 ? 4096 : vertexBufferCapacity_;
    while (newCapacity < vertexCount) {
        newCapacity *= 2;
    }

    D3D11_BUFFER_DESC desc{};
    desc.ByteWidth = static_cast<UINT>(newCapacity * sizeof(Vertex));
    desc.Usage = D3D11_USAGE_DYNAMIC;
    desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

    vertexBuffer_.Reset();
    HRESULT hr = device_->CreateBuffer(&desc, nullptr, vertexBuffer_.GetAddressOf());
    if (FAILED(hr)) {
        vertexBufferCapacity_ = 0;
        return false;
    }
    vertexBufferCapacity_ = newCapacity;
    return true;
}

bool RendererDX11::ensureTextVertexBufferCapacity(size_t vertexCount) {
    if (vertexCount <= textVertexBufferCapacity_) {
        return true;
    }

    size_t newCapacity = textVertexBufferCapacity_ == 0 ? 4096 : textVertexBufferCapacity_;
    while (newCapacity < vertexCount) {
        newCapacity *= 2;
    }

    D3D11_BUFFER_DESC desc{};
    desc.ByteWidth = static_cast<UINT>(newCapacity * sizeof(TextVertex));
    desc.Usage = D3D11_USAGE_DYNAMIC;
    desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

    textVertexBuffer_.Reset();
    HRESULT hr = device_->CreateBuffer(&desc, nullptr, textVertexBuffer_.GetAddressOf());
    if (FAILED(hr)) {
        textVertexBufferCapacity_ = 0;
        return false;
    }
    textVertexBufferCapacity_ = newCapacity;
    return true;
}

void RendererDX11::resize(uint32_t width, uint32_t height) {
    if (!swapChain_ || width == 0 || height == 0) {
        return;
    }
    if (width == width_ && height == height_) {
        return;
    }

    releaseRenderTarget();
    HRESULT hr = swapChain_->ResizeBuffers(0, width, height, DXGI_FORMAT_UNKNOWN, 0);
    if (FAILED(hr)) {
        return;
    }

    width_ = width;
    height_ = height;
    createRenderTarget();
}

void RendererDX11::beginFrame(const Color4& clearColor) {
    pendingVertices_.clear();
    pendingTextVertices_.clear();

    if (!context_ || !renderTargetView_) {
        return;
    }

    const float clear[4] = {clearColor.r, clearColor.g, clearColor.b, clearColor.a};
    context_->ClearRenderTargetView(renderTargetView_.Get(), clear);
}

void RendererDX11::drawRect(float x, float y, float w, float h, const Color4& color) {
    if (width_ == 0 || height_ == 0 || w <= 0.0f || h <= 0.0f) {
        return;
    }

    // Pixel space (origin top-left) -> NDC (origin center, +Y up).
    const float fw = static_cast<float>(width_);
    const float fh = static_cast<float>(height_);
    const float x0 = (x / fw) * 2.0f - 1.0f;
    const float x1 = ((x + w) / fw) * 2.0f - 1.0f;
    const float y0 = 1.0f - (y / fh) * 2.0f;
    const float y1 = 1.0f - ((y + h) / fh) * 2.0f;

    Vertex topLeft{x0, y0, color.r, color.g, color.b, color.a};
    Vertex topRight{x1, y0, color.r, color.g, color.b, color.a};
    Vertex bottomLeft{x0, y1, color.r, color.g, color.b, color.a};
    Vertex bottomRight{x1, y1, color.r, color.g, color.b, color.a};

    // Two triangles, matching D3D11's default clockwise front-face winding.
    pendingVertices_.push_back(topLeft);
    pendingVertices_.push_back(topRight);
    pendingVertices_.push_back(bottomLeft);
    pendingVertices_.push_back(topRight);
    pendingVertices_.push_back(bottomRight);
    pendingVertices_.push_back(bottomLeft);
}

void RendererDX11::drawText(float x, float y, const std::string& text, float scale, const Color4& color) {
    if (width_ == 0 || height_ == 0 || scale <= 0.0f) {
        return;
    }

    const float charW = fontCharWidth_ * scale;
    const float charH = fontCharHeight_ * scale;
    const auto glyphs = layoutText(text, x, y, charW, charH);

    const float fw = static_cast<float>(width_);
    const float fh = static_cast<float>(height_);
    for (const auto& g : glyphs) {
        const float x0 = (g.x / fw) * 2.0f - 1.0f;
        const float x1 = ((g.x + g.w) / fw) * 2.0f - 1.0f;
        const float y0 = 1.0f - (g.y / fh) * 2.0f;
        const float y1 = 1.0f - ((g.y + g.h) / fh) * 2.0f;

        TextVertex topLeft{x0, y0, g.u0, g.v0, color.r, color.g, color.b, color.a};
        TextVertex topRight{x1, y0, g.u1, g.v0, color.r, color.g, color.b, color.a};
        TextVertex bottomLeft{x0, y1, g.u0, g.v1, color.r, color.g, color.b, color.a};
        TextVertex bottomRight{x1, y1, g.u1, g.v1, color.r, color.g, color.b, color.a};

        pendingTextVertices_.push_back(topLeft);
        pendingTextVertices_.push_back(topRight);
        pendingTextVertices_.push_back(bottomLeft);
        pendingTextVertices_.push_back(topRight);
        pendingTextVertices_.push_back(bottomRight);
        pendingTextVertices_.push_back(bottomLeft);
    }
}

void RendererDX11::endFrame() {
    if (!context_ || !renderTargetView_) {
        return;
    }

    if (!pendingVertices_.empty() && ensureVertexBufferCapacity(pendingVertices_.size())) {
        D3D11_MAPPED_SUBRESOURCE mapped{};
        if (SUCCEEDED(context_->Map(vertexBuffer_.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped))) {
            memcpy(mapped.pData, pendingVertices_.data(), pendingVertices_.size() * sizeof(Vertex));
            context_->Unmap(vertexBuffer_.Get(), 0);
        }
    }

    if (!pendingTextVertices_.empty() && ensureTextVertexBufferCapacity(pendingTextVertices_.size())) {
        D3D11_MAPPED_SUBRESOURCE mapped{};
        if (SUCCEEDED(context_->Map(textVertexBuffer_.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped))) {
            memcpy(mapped.pData, pendingTextVertices_.data(), pendingTextVertices_.size() * sizeof(TextVertex));
            context_->Unmap(textVertexBuffer_.Get(), 0);
        }
    }

    ID3D11RenderTargetView* rtv = renderTargetView_.Get();
    context_->OMSetRenderTargets(1, &rtv, nullptr);

    D3D11_VIEWPORT viewport{};
    viewport.Width = static_cast<float>(width_);
    viewport.Height = static_cast<float>(height_);
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;
    context_->RSSetViewports(1, &viewport);

    const float blendFactor[4] = {0.0f, 0.0f, 0.0f, 0.0f};
    context_->OMSetBlendState(alphaBlendState_.Get(), blendFactor, 0xFFFFFFFF);

    if (!pendingVertices_.empty() && vertexBuffer_) {
        const UINT stride = sizeof(Vertex);
        const UINT offset = 0;
        ID3D11Buffer* vb = vertexBuffer_.Get();
        context_->IASetVertexBuffers(0, 1, &vb, &stride, &offset);
        context_->IASetInputLayout(inputLayout_.Get());
        context_->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        context_->VSSetShader(vertexShader_.Get(), nullptr, 0);
        context_->PSSetShader(pixelShader_.Get(), nullptr, 0);
        context_->Draw(static_cast<UINT>(pendingVertices_.size()), 0);
    }

    if (!pendingTextVertices_.empty() && textVertexBuffer_) {
        const UINT stride = sizeof(TextVertex);
        const UINT offset = 0;
        ID3D11Buffer* vb = textVertexBuffer_.Get();
        context_->IASetVertexBuffers(0, 1, &vb, &stride, &offset);
        context_->IASetInputLayout(textInputLayout_.Get());
        context_->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        context_->VSSetShader(textVertexShader_.Get(), nullptr, 0);
        context_->PSSetShader(textPixelShader_.Get(), nullptr, 0);
        ID3D11ShaderResourceView* srv = fontAtlasSRV_.Get();
        context_->PSSetShaderResources(0, 1, &srv);
        ID3D11SamplerState* sampler = fontSamplerState_.Get();
        context_->PSSetSamplers(0, 1, &sampler);
        context_->Draw(static_cast<UINT>(pendingTextVertices_.size()), 0);
    }

    swapChain_->Present(1, 0);
    pendingVertices_.clear();
    pendingTextVertices_.clear();
}

}  // namespace app
