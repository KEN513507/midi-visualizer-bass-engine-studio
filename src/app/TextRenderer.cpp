#include "TextRenderer.h"

namespace app {

std::vector<GlyphQuad> layoutText(const std::string& text, float x, float y, float charWidth, float charHeight) {
    std::vector<GlyphQuad> quads;
    quads.reserve(text.size());

    float cursorX = x;
    for (unsigned char c : text) {
        // Space (and anything outside the printable range) advances the
        // cursor with no visible glyph.
        if (c > static_cast<unsigned char>(kFontFirstAsciiChar) && c <= static_cast<unsigned char>(kFontLastAsciiChar)) {
            const int glyphIndex = c - kFontFirstAsciiChar;
            const int col = glyphIndex % kFontAtlasCols;
            const int row = glyphIndex / kFontAtlasCols;

            GlyphQuad quad;
            quad.x = cursorX;
            quad.y = y;
            quad.w = charWidth;
            quad.h = charHeight;
            quad.u0 = static_cast<float>(col) / static_cast<float>(kFontAtlasCols);
            quad.v0 = static_cast<float>(row) / static_cast<float>(kFontAtlasRows);
            quad.u1 = static_cast<float>(col + 1) / static_cast<float>(kFontAtlasCols);
            quad.v1 = static_cast<float>(row + 1) / static_cast<float>(kFontAtlasRows);
            quads.push_back(quad);
        }
        cursorX += charWidth;
    }

    return quads;
}

}  // namespace app
