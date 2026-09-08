#include "PianoLayout.h"

#include <array>

namespace app {

namespace {
constexpr std::array<bool, 12> kIsWhitePitchClass = {
    true,   // C
    false,  // C#
    true,   // D
    false,  // D#
    true,   // E
    true,   // F
    false,  // F#
    true,   // G
    false,  // G#
    true,   // A
    false,  // A#
    true,   // B
};

// Black keys read narrower than white keys on a real instrument.
constexpr float kBlackKeyWidthRatio = 0.6f;
}  // namespace

bool isWhiteKey(int midiNote) {
    return kIsWhitePitchClass[((midiNote % 12) + 12) % 12];
}

bool isBlackKey(int midiNote) {
    return !isWhiteKey(midiNote);
}

std::vector<PianoKeyRect> computePianoLayout(float totalWidthPx) {
    std::vector<PianoKeyRect> layout;
    if (totalWidthPx <= 0.0f) {
        return layout;
    }

    int whiteKeyCount = 0;
    for (int note = kPianoLowestMidiNote; note <= kPianoHighestMidiNote; ++note) {
        if (isWhiteKey(note)) {
            ++whiteKeyCount;
        }
    }
    if (whiteKeyCount == 0) {
        return layout;
    }

    const float whiteWidth = totalWidthPx / static_cast<float>(whiteKeyCount);
    const float blackWidth = whiteWidth * kBlackKeyWidthRatio;

    layout.reserve(kPianoKeyCount);
    int whiteIndex = 0;
    for (int note = kPianoLowestMidiNote; note <= kPianoHighestMidiNote; ++note) {
        PianoKeyRect rect;
        rect.midiNote = note;
        if (isWhiteKey(note)) {
            rect.isBlack = false;
            rect.width = whiteWidth;
            rect.x = static_cast<float>(whiteIndex) * whiteWidth;
            ++whiteIndex;
        } else {
            // Black keys sit on the shared boundary between the white key
            // just placed and the one about to be placed, matching real
            // piano geometry (never present between E/F or B/C, which is
            // already guaranteed by kIsWhitePitchClass).
            const float boundary = static_cast<float>(whiteIndex) * whiteWidth;
            rect.isBlack = true;
            rect.width = blackWidth;
            rect.x = boundary - blackWidth * 0.5f;
        }
        layout.push_back(rect);
    }

    return layout;
}

}  // namespace app
