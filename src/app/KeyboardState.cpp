#include "KeyboardState.h"

namespace app {

namespace {
bool isValidMidiNote(int note) {
    return note >= 0 && note <= 127;
}
}  // namespace

void KeyboardState::apply(const midi::MidiEventProcessor::ProcessedEvents& events,
                           trainer::RootlessTrainer& trainer) {
    for (const auto& note : events.noteOn) {
        if (!isValidMidiNote(note.midiNote)) {
            continue;
        }
        heldPhysically_[note.midiNote] = true;
        noteStates_[note.midiNote].sounding = true;
        if (trainer.onNoteOn(note.midiNote, note.velocity).has_value()) {
            noteStates_[note.midiNote].violation = true;
        }
    }

    for (const auto& note : events.noteOff) {
        if (!isValidMidiNote(note.midiNote)) {
            continue;
        }
        heldPhysically_[note.midiNote] = false;
        if (!sustainOn_) {
            noteStates_[note.midiNote].sounding = false;
            noteStates_[note.midiNote].violation = false;
        }
    }

    for (const auto& sustain : events.sustain) {
        sustainOn_ = sustain.on;
        if (!sustainOn_) {
            // Pedal release: anything not still physically held stops
            // sounding. Physically-held notes are unaffected by sustain.
            for (int note = 0; note < 128; ++note) {
                if (!heldPhysically_[note]) {
                    noteStates_[note].sounding = false;
                    noteStates_[note].violation = false;
                }
            }
        }
    }
}

}  // namespace app
