#include <cmath>

#include "TestFramework.h"
#include "app/TextRenderer.h"

using namespace app;

TEST_CASE(TextRenderer_SpaceProducesNoGlyphButAdvancesCursor) {
    const auto quads = layoutText("A B", 0.0f, 0.0f, 10.0f, 16.0f);
    // 'A' and 'B' produce glyphs; the space between does not.
    REQUIRE_EQ(quads.size(), static_cast<size_t>(2));
    REQUIRE(std::abs(quads[0].x - 0.0f) < 0.001f);
    REQUIRE(std::abs(quads[1].x - 20.0f) < 0.001f);  // 'A'(0) space(10) 'B'(20)
}

TEST_CASE(TextRenderer_GlyphsAreMonospaceAdvanced) {
    const auto quads = layoutText("XY", 100.0f, 50.0f, 12.0f, 20.0f);
    REQUIRE_EQ(quads.size(), static_cast<size_t>(2));
    REQUIRE(std::abs(quads[0].x - 100.0f) < 0.001f);
    REQUIRE(std::abs(quads[0].y - 50.0f) < 0.001f);
    REQUIRE(std::abs(quads[0].w - 12.0f) < 0.001f);
    REQUIRE(std::abs(quads[0].h - 20.0f) < 0.001f);
    REQUIRE(std::abs(quads[1].x - 112.0f) < 0.001f);
}

TEST_CASE(TextRenderer_UvRectsAreWithinAtlasBounds) {
    const auto quads = layoutText("~!A0", 0.0f, 0.0f, 8.0f, 8.0f);
    for (const auto& q : quads) {
        REQUIRE(q.u0 >= 0.0f);
        REQUIRE(q.v0 >= 0.0f);
        REQUIRE(q.u1 <= 1.0f);
        REQUIRE(q.v1 <= 1.0f);
        REQUIRE(q.u1 > q.u0);
        REQUIRE(q.v1 > q.v0);
    }
}

TEST_CASE(TextRenderer_DistinctCharactersGetDistinctUv) {
    const auto quads = layoutText("AB", 0.0f, 0.0f, 8.0f, 8.0f);
    REQUIRE_EQ(quads.size(), static_cast<size_t>(2));
    REQUIRE_FALSE(std::abs(quads[0].u0 - quads[1].u0) < 0.0001f && std::abs(quads[0].v0 - quads[1].v0) < 0.0001f);
}

TEST_CASE(TextRenderer_EmptyString_ProducesNoQuads) {
    REQUIRE(layoutText("", 0.0f, 0.0f, 8.0f, 8.0f).empty());
}
