#include "TestFramework.h"
#include "app/KeyboardState.h"

using namespace app;
using midi::MidiEventProcessor;
using midi::NoteEvent;
using midi::SustainEvent;

namespace {
trainer::RootlessTrainer makeTrainerNoViolationPossible() {
    trainer::RootlessTrainer t;
    t.clearAllMonitoredOctaves();
    t.setCurrentChord(timeline::kInvalidPitchClass, /*isNoChord=*/true);
    return t;
}
}  // namespace

TEST_CASE(KeyboardState_NoteOn_MarksSounding) {
    KeyboardState state;
    auto trainer = makeTrainerNoViolationPossible();

    MidiEventProcessor::ProcessedEvents events;
    events.noteOn.push_back(NoteEvent{1, 60, 100, 0});
    state.apply(events, trainer);

    REQUIRE(state.noteStates()[60].sounding);
    REQUIRE_FALSE(state.noteStates()[60].violation);
}

TEST_CASE(KeyboardState_NoteOff_ClearsSounding_WithoutSustain) {
    KeyboardState state;
    auto trainer = makeTrainerNoViolationPossible();

    MidiEventProcessor::ProcessedEvents onEvents;
    onEvents.noteOn.push_back(NoteEvent{1, 60, 100, 0});
    state.apply(onEvents, trainer);

    MidiEventProcessor::ProcessedEvents offEvents;
    offEvents.noteOff.push_back(NoteEvent{1, 60, 0, 1});
    state.apply(offEvents, trainer);

    REQUIRE_FALSE(state.noteStates()[60].sounding);
}

TEST_CASE(KeyboardState_NoteOff_WithSustainHeld_KeepsSounding) {
    KeyboardState state;
    auto trainer = makeTrainerNoViolationPossible();

    MidiEventProcessor::ProcessedEvents onEvents;
    onEvents.noteOn.push_back(NoteEvent{1, 60, 100, 0});
    onEvents.sustain.push_back(SustainEvent{1, true, 0});
    state.apply(onEvents, trainer);

    MidiEventProcessor::ProcessedEvents offEvents;
    offEvents.noteOff.push_back(NoteEvent{1, 60, 0, 1});
    state.apply(offEvents, trainer);

    // Pedal is still down: the released key must keep sounding visually.
    REQUIRE(state.noteStates()[60].sounding);
}

TEST_CASE(KeyboardState_SustainRelease_StopsNonHeldNotes) {
    KeyboardState state;
    auto trainer = makeTrainerNoViolationPossible();

    MidiEventProcessor::ProcessedEvents onEvents;
    onEvents.noteOn.push_back(NoteEvent{1, 60, 100, 0});
    onEvents.sustain.push_back(SustainEvent{1, true, 0});
    state.apply(onEvents, trainer);

    MidiEventProcessor::ProcessedEvents offEvents;
    offEvents.noteOff.push_back(NoteEvent{1, 60, 0, 1});
    state.apply(offEvents, trainer);
    REQUIRE(state.noteStates()[60].sounding);

    MidiEventProcessor::ProcessedEvents sustainOff;
    sustainOff.sustain.push_back(SustainEvent{1, false, 2});
    state.apply(sustainOff, trainer);

    REQUIRE_FALSE(state.noteStates()[60].sounding);
}

TEST_CASE(KeyboardState_SustainRelease_DoesNotStopPhysicallyHeldNotes) {
    KeyboardState state;
    auto trainer = makeTrainerNoViolationPossible();

    MidiEventProcessor::ProcessedEvents onEvents;
    onEvents.noteOn.push_back(NoteEvent{1, 60, 100, 0});
    onEvents.sustain.push_back(SustainEvent{1, true, 0});
    state.apply(onEvents, trainer);

    MidiEventProcessor::ProcessedEvents sustainOff;
    sustainOff.sustain.push_back(SustainEvent{1, false, 1});
    state.apply(sustainOff, trainer);

    // Note 60 is still physically held (no NoteOff was ever sent).
    REQUIRE(state.noteStates()[60].sounding);
}

TEST_CASE(KeyboardState_RootlessViolation_MarksAndClearsOnRelease) {
    KeyboardState state;
    trainer::RootlessTrainer trainer;
    trainer.applyOct3To5Preset();
    trainer.setCurrentChord(/*root=*/0 /* C */, /*isNoChord=*/false);

    MidiEventProcessor::ProcessedEvents onEvents;
    onEvents.noteOn.push_back(NoteEvent{1, 60, 100, 0});  // C4, monitored octave, root pitch class
    state.apply(onEvents, trainer);

    REQUIRE(state.noteStates()[60].sounding);
    REQUIRE(state.noteStates()[60].violation);

    MidiEventProcessor::ProcessedEvents offEvents;
    offEvents.noteOff.push_back(NoteEvent{1, 60, 0, 1});
    state.apply(offEvents, trainer);

    REQUIRE_FALSE(state.noteStates()[60].sounding);
    REQUIRE_FALSE(state.noteStates()[60].violation);
}
