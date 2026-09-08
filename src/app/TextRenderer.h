#pragma once

#include <string>
#include <vector>

namespace app {

// Font atlas layout shared between glyph placement (pure, tested here) and
// atlas texture generation (GDI-specific, RendererDX11.cpp). Printable
// ASCII 32 ('space') through 126 ('~') = 95 glyphs, laid out left-to-right,
// top-to-bottom in a fixed grid with one spare cell.
constexpr int kFontAtlasCols = 16;
constexpr int kFontAtlasRows = 6;
constexpr int kFontFirstAsciiChar = 32;
constexpr int kFontLastAsciiChar = 126;

struct GlyphQuad {
    float x = 0.0f, y = 0.0f, w = 0.0f, h = 0.0f;  // screen-space pixel rect
    float u0 = 0.0f, v0 = 0.0f, u1 = 0.0f, v1 = 0.0f;  // atlas UV rect
};

// Lays out `text` as a single-line horizontal monospace run starting at
// (x, y). Every character advances the cursor by charWidth, including
// unsupported ones (treated as a blank space) so alignment of any text
// after it is preserved; only glyphs with visible ink (33..126, i.e.
// excluding space) produce a GlyphQuad in the returned vector, so its size
// may be less than text.size(). Callers needing multiple lines call this
// once per line.
std::vector<GlyphQuad> layoutText(const std::string& text, float x, float y, float charWidth, float charHeight);

}  // namespace app
