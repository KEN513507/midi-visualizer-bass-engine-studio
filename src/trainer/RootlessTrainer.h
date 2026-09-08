#pragma once

#include <array>
#include <optional>

#include "timeline/Types.h"

namespace trainer {

// docs/SYSTEM_REQUIREMENTS.md section 17: rootless violation evaluation
// occurs only on a new physical USB-MIDI NoteOn. Held notes crossing a
// chord boundary or CC64 sustain must not re-trigger a violation, which
// this design guarantees structurally: the only entry point that can
// produce a Violation is onNoteOn().
class RootlessTrainer {
public:
    // Octaves 1 through 7, individually configurable.
    static constexpr int kMinOctave = 1;
    static constexpr int kMaxOctave = 7;

    RootlessTrainer();

    void setMonitoredOctave(int octave, bool enabled);
    bool isOctaveMonitored(int octave) const;

    // Preset required by the UX spec: Oct 3 through 5.
    void applyOct3To5Preset();

    void clearAllMonitoredOctaves();

    // Called by the timeline/playback layer whenever the active chord
    // changes. isNoChord suppresses all violations (N.C. has no harmonic
    // root, per docs/SYSTEM_REQUIREMENTS.md section 11).
    void setCurrentChord(timeline::PitchClass root, bool isNoChord);

    struct Violation {
        int midiNote = 0;
        int velocity = 0;
        timeline::PitchClass offendingPitchClass = timeline::kInvalidPitchClass;
        int octave = 0;
    };

    // Must be called only for a genuine new physical NoteOn (velocity > 0),
    // never for a sustain-held repeat or a boundary re-check.
    std::optional<Violation> onNoteOn(int midiNote, int velocity);

private:
    // Octave for a MIDI note under the fixed MIDI60=C4 convention.
    static int octaveOf(int midiNote);
    static timeline::PitchClass pitchClassOf(int midiNote);

    std::array<bool, kMaxOctave - kMinOctave + 1> monitoredOctaves_{};
    timeline::PitchClass currentRoot_ = timeline::kInvalidPitchClass;
    bool currentIsNoChord_ = true;
};

}  // namespace trainer
