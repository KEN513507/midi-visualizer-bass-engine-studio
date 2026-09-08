#pragma once

#include <array>

#include "midi/MidiEventProcessor.h"
#include "trainer/RootlessTrainer.h"

namespace app {

// Per-MIDI-note (0-127) visual state driven purely by processed MIDI
// events and the RootlessTrainer, independent of DX11 so it is directly
// unit-testable (docs/UI_REQUIREMENTS.md section 16-18: waterfall/piano
// note highlighting and rootless violation feedback).
struct NoteVisualState {
    bool sounding = false;   // key should render as actively sounding
    bool violation = false;  // key is a live rootless-trainer violation
};

// Applies CC64 sustain semantics on top of raw NoteOn/NoteOff so a
// physically released key keeps sounding while the pedal is held, matching
// real piano behavior and the trainer's documented contract that only a
// genuine physical NoteOn may produce a Violation.
class KeyboardState {
public:
    void apply(const midi::MidiEventProcessor::ProcessedEvents& events, trainer::RootlessTrainer& trainer);

    const std::array<NoteVisualState, 128>& noteStates() const { return noteStates_; }

private:
    std::array<NoteVisualState, 128> noteStates_{};
    std::array<bool, 128> heldPhysically_{};
    bool sustainOn_ = false;
};

}  // namespace app
