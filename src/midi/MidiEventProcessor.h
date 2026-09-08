#pragma once

#include <cstdint>
#include <vector>

#include "MidiTypes.h"

namespace midi {

// Bridges raw MidiShortMessage records (as produced by MidiInput) into the
// discrete semantic events the application layer (RootlessTrainer,
// waterfall, timeline playback) actually consumes. This is the
// "MidiInput abstraction -> application state" step in the bring-up
// dependency flow (docs/USB_MIDI_BRINGUP_TEST.md section 15) and is kept
// hardware-independent so it is directly unit-testable.
struct NoteEvent {
    int channel = 1;
    int midiNote = 0;
    int velocity = 0;
    uint32_t timestampMs = 0;
};

struct SustainEvent {
    int channel = 1;
    bool on = false;
    uint32_t timestampMs = 0;
};

class MidiEventProcessor {
public:
    struct ProcessedEvents {
        std::vector<NoteEvent> noteOn;
        std::vector<NoteEvent> noteOff;
        std::vector<SustainEvent> sustain;
    };

    // A Note On with velocity zero is normalized to Note Off here, so
    // callers never need to special-case it.
    ProcessedEvents process(const std::vector<MidiShortMessage>& messages) const;
};

}  // namespace midi
