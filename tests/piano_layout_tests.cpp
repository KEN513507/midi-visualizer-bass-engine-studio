#include <cmath>

#include "TestFramework.h"
#include "app/PianoLayout.h"

using namespace app;

TEST_CASE(Piano_Range_IsA0ToC8_88Keys) {
    REQUIRE_EQ(kPianoLowestMidiNote, 21);
    REQUIRE_EQ(kPianoHighestMidiNote, 108);
    REQUIRE_EQ(kPianoKeyCount, 88);
}

TEST_CASE(Piano_MiddleCIsWhite) {
    // docs/UI_REQUIREMENTS.md section 17: Middle C is C4 / MIDI 60.
    REQUIRE(isWhiteKey(60));
    REQUIRE_FALSE(isBlackKey(60));
}

TEST_CASE(Piano_KnownWhiteAndBlackPitchClasses) {
    // C D E F G A B are white; C# D# F# G# A# are black.
    const int whiteNotes[] = {60, 62, 64, 65, 67, 69, 71};
    for (int n : whiteNotes) {
        REQUIRE(isWhiteKey(n));
    }
    const int blackNotes[] = {61, 63, 66, 68, 70};
    for (int n : blackNotes) {
        REQUIRE(isBlackKey(n));
    }
}

TEST_CASE(Piano_ComputeLayout_ProducesEntryPerKey_InRangeOrder) {
    const auto layout = computePianoLayout(1000.0f);
    REQUIRE_EQ(layout.size(), static_cast<size_t>(kPianoKeyCount));
    for (size_t i = 0; i < layout.size(); ++i) {
        REQUIRE_EQ(layout[i].midiNote, kPianoLowestMidiNote + static_cast<int>(i));
    }
}

TEST_CASE(Piano_ComputeLayout_WhiteKeysSpanFullWidthContiguously) {
    const float totalWidth = 1040.0f;  // 52 white keys * 20px
    const auto layout = computePianoLayout(totalWidth);

    float expectedNextX = 0.0f;
    for (const auto& rect : layout) {
        if (!rect.isBlack) {
            REQUIRE(std::abs(rect.x - expectedNextX) < 0.001f);
            expectedNextX += rect.width;
        }
    }
    REQUIRE(std::abs(expectedNextX - totalWidth) < 0.01f);
}

TEST_CASE(Piano_ComputeLayout_BlackKeysNarrowerAndWithinBounds) {
    const float totalWidth = 1040.0f;
    const auto layout = computePianoLayout(totalWidth);

    float whiteWidth = 0.0f;
    for (const auto& rect : layout) {
        if (!rect.isBlack) {
            whiteWidth = rect.width;
            break;
        }
    }

    for (const auto& rect : layout) {
        if (rect.isBlack) {
            REQUIRE(rect.width < whiteWidth);
            REQUIRE(rect.x >= 0.0f);
            REQUIRE(rect.x + rect.width <= totalWidth);
        }
    }
}

TEST_CASE(Piano_ComputeLayout_ZeroOrNegativeWidth_ReturnsEmpty) {
    REQUIRE(computePianoLayout(0.0f).empty());
    REQUIRE(computePianoLayout(-100.0f).empty());
}
