#pragma once

#include <vector>

namespace app {

// docs/UI_REQUIREMENTS.md section 17: standard A0-C8 range (MIDI 21-108),
// white/black key geometry and octave positions must be musically correct.
// Pure geometry, independent of DX11, so it is unit-testable and reusable
// for waterfall-to-key horizontal alignment (WATERFALL_ALIGNMENT_GATE).
constexpr int kPianoLowestMidiNote = 21;   // A0
constexpr int kPianoHighestMidiNote = 108;  // C8
constexpr int kPianoKeyCount = kPianoHighestMidiNote - kPianoLowestMidiNote + 1;  // 88

bool isWhiteKey(int midiNote);
bool isBlackKey(int midiNote);

struct PianoKeyRect {
    int midiNote = 0;
    float x = 0.0f;      // left edge, pixels from the piano's left origin
    float width = 0.0f;  // pixels
    bool isBlack = false;
};

// Lays out all 88 keys left-to-right across totalWidthPx. White keys are
// laid out first at uniform width; black keys are centered on the shared
// boundary between their neighboring white keys (standard piano geometry),
// narrower and drawn on top. Returns an empty vector for a non-positive
// width (e.g. a minimized window).
std::vector<PianoKeyRect> computePianoLayout(float totalWidthPx);

}  // namespace app
