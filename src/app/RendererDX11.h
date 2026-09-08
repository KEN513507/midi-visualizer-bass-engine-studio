#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <windows.h>
#include <wrl/client.h>
#include <d3d11.h>

namespace app {

struct Color4 {
    float r = 0.0f;
    float g = 0.0f;
    float b = 0.0f;
    float a = 1.0f;
};

// DirectX 11 device/swapchain/render-target lifecycle. Frame timing here is
// display-rate only; it must never be treated as the music clock
// (docs/SYSTEM_REQUIREMENTS.md section 3/19 — WASAPI is authoritative).
//
// Also owns a minimal immediate-mode flat-color quad renderer (a single
// pass-through shader pipeline + dynamic vertex buffer) so callers can lay
// out UI regions (header/timeline/waterfall/piano/footer, piano keys) as
// screen-space pixel rectangles without hand-rolling DX11 per call site.
class RendererDX11 {
public:
    bool initialize(HWND hwnd, uint32_t width, uint32_t height);
    void shutdown();

    // Recreates the swapchain buffers for a new client size. No-op for a
    // zero-sized (minimized) window.
    void resize(uint32_t width, uint32_t height);

    // Clears the backbuffer and resets the accumulated quad list for a new
    // frame. Call drawRect() any number of times, then endFrame().
    void beginFrame(const Color4& clearColor);

    // Queues an axis-aligned filled rectangle in client pixel coordinates
    // (origin top-left, matching Win32 client area convention). Actually
    // drawn on endFrame().
    void drawRect(float x, float y, float w, float h, const Color4& color);

    // Queues a line of monospace text at (x, y) in client pixel
    // coordinates, using the built-in GDI-generated font atlas. scale 1.0
    // renders at the atlas's native cell size (fontCharWidth()/
    // fontCharHeight()); text with characters outside printable ASCII is
    // rendered with those positions left blank (see TextRenderer.h).
    void drawText(float x, float y, const std::string& text, float scale, const Color4& color);

    float fontCharWidth() const { return fontCharWidth_; }
    float fontCharHeight() const { return fontCharHeight_; }

    // Uploads the queued quads/text, draws them, and presents the frame.
    void endFrame();

    bool isInitialized() const { return device_ != nullptr; }

private:
    bool createRenderTarget();
    void releaseRenderTarget();
    bool createPipeline();
    bool createFontAtlas();
    bool createTextPipeline();
    bool ensureVertexBufferCapacity(size_t vertexCount);
    bool ensureTextVertexBufferCapacity(size_t vertexCount);

    struct Vertex {
        float x = 0.0f;
        float y = 0.0f;
        float r = 0.0f;
        float g = 0.0f;
        float b = 0.0f;
        float a = 1.0f;
    };

    struct TextVertex {
        float x = 0.0f;
        float y = 0.0f;
        float u = 0.0f;
        float v = 0.0f;
        float r = 0.0f;
        float g = 0.0f;
        float b = 0.0f;
        float a = 1.0f;
    };

    Microsoft::WRL::ComPtr<ID3D11Device> device_;
    Microsoft::WRL::ComPtr<ID3D11DeviceContext> context_;
    Microsoft::WRL::ComPtr<IDXGISwapChain> swapChain_;
    Microsoft::WRL::ComPtr<ID3D11RenderTargetView> renderTargetView_;

    Microsoft::WRL::ComPtr<ID3D11VertexShader> vertexShader_;
    Microsoft::WRL::ComPtr<ID3D11PixelShader> pixelShader_;
    Microsoft::WRL::ComPtr<ID3D11InputLayout> inputLayout_;
    Microsoft::WRL::ComPtr<ID3D11Buffer> vertexBuffer_;
    size_t vertexBufferCapacity_ = 0;

    Microsoft::WRL::ComPtr<ID3D11VertexShader> textVertexShader_;
    Microsoft::WRL::ComPtr<ID3D11PixelShader> textPixelShader_;
    Microsoft::WRL::ComPtr<ID3D11InputLayout> textInputLayout_;
    Microsoft::WRL::ComPtr<ID3D11Buffer> textVertexBuffer_;
    size_t textVertexBufferCapacity_ = 0;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> fontAtlasSRV_;
    Microsoft::WRL::ComPtr<ID3D11SamplerState> fontSamplerState_;
    Microsoft::WRL::ComPtr<ID3D11BlendState> alphaBlendState_;
    float fontCharWidth_ = 0.0f;
    float fontCharHeight_ = 0.0f;

    std::vector<Vertex> pendingVertices_;
    std::vector<TextVertex> pendingTextVertices_;

    uint32_t width_ = 0;
    uint32_t height_ = 0;
};

}  // namespace app
