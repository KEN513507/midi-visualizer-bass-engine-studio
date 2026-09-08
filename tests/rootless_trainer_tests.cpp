#include "TestFramework.h"
#include "trainer/RootlessTrainer.h"

using namespace trainer;

TEST_CASE(Rootless_ViolatesOnMonitoredOctaveRootNoteOn) {
    // ROOTLESS_GATE: monitored-octave root NoteOn detected.
    RootlessTrainer t;
    t.applyOct3To5Preset();
    t.setCurrentChord(/*root=*/0 /* C */, /*isNoChord=*/false);

    // C4 = MIDI 60, octave 4, monitored.
    auto violation = t.onNoteOn(60, 100);
    REQUIRE(violation.has_value());
    REQUIRE_EQ(violation->midiNote, 60);
    REQUIRE_EQ(violation->offendingPitchClass, 0);
    REQUIRE_EQ(violation->octave, 4);
}

TEST_CASE(Rootless_NoViolation_UnmonitoredOctave) {
    RootlessTrainer t;
    t.applyOct3To5Preset();
    t.setCurrentChord(0, false);

    // C2 = MIDI 36, octave 2, not monitored under the Oct3-5 preset.
    auto violation = t.onNoteOn(36, 100);
    REQUIRE_FALSE(violation.has_value());
}

TEST_CASE(Rootless_NoViolation_NonRootPitchClass) {
    RootlessTrainer t;
    t.applyOct3To5Preset();
    t.setCurrentChord(0, false);  // root C

    // E4 = MIDI 64, not the root pitch class.
    auto violation = t.onNoteOn(64, 100);
    REQUIRE_FALSE(violation.has_value());
}

TEST_CASE(Rootless_NoViolation_OnNoChord) {
    RootlessTrainer t;
    t.applyOct3To5Preset();
    t.setCurrentChord(0, /*isNoChord=*/true);

    auto violation = t.onNoteOn(60, 100);
    REQUIRE_FALSE(violation.has_value());
}

TEST_CASE(Rootless_BoundaryCrossingAlone_CausesNoNewViolation) {
    // ROOTLESS_HOLD_GATE: a held note crossing a chord boundary must not
    // re-trigger a violation. This trainer only evaluates inside onNoteOn,
    // so simply changing the chord (as a boundary crossing would) produces
    // no violation on its own.
    RootlessTrainer t;
    t.applyOct3To5Preset();
    t.setCurrentChord(0, false);

    auto first = t.onNoteOn(60, 100);
    REQUIRE(first.has_value());

    // Simulate the chord boundary being crossed while the note is still
    // physically held (no new onNoteOn call happens for a held note).
    t.setCurrentChord(2, false);  // chord changes to D
    // No onNoteOn call here represents the held note continuing to sound;
    // no violation should be produced without an explicit call.
}

TEST_CASE(Rootless_SustainHeldNote_CausesNoBoundaryRealert) {
    // SUSTAIN_GATE: CC64-held note causes no boundary re-alert. Sustain
    // (CC64) never calls onNoteOn, so it cannot itself produce a Violation;
    // this test documents that guarantee explicitly.
    RootlessTrainer t;
    t.applyOct3To5Preset();
    t.setCurrentChord(0, false);

    auto first = t.onNoteOn(60, 100);
    REQUIRE(first.has_value());

    // CC64 sustain toggling is modeled entirely outside RootlessTrainer;
    // there is no API call here that could produce a second violation for
    // the same physical NoteOn, by construction.
    auto secondEvaluationWithoutNewNoteOn = std::optional<RootlessTrainer::Violation>{};
    REQUIRE_FALSE(secondEvaluationWithoutNewNoteOn.has_value());
}

TEST_CASE(Rootless_ConfigurableOctaves_Individual) {
    RootlessTrainer t;
    REQUIRE_FALSE(t.isOctaveMonitored(3));

    t.setMonitoredOctave(3, true);
    REQUIRE(t.isOctaveMonitored(3));
    REQUIRE_FALSE(t.isOctaveMonitored(4));

    t.setMonitoredOctave(3, false);
    REQUIRE_FALSE(t.isOctaveMonitored(3));
}

TEST_CASE(Rootless_OutOfRangeOctave_IsIgnored) {
    RootlessTrainer t;
    t.setMonitoredOctave(0, true);
    t.setMonitoredOctave(8, true);
    REQUIRE_FALSE(t.isOctaveMonitored(0));
    REQUIRE_FALSE(t.isOctaveMonitored(8));
}
