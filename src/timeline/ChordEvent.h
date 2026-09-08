#pragma once

#include <string>

#include "Types.h"

namespace timeline {

// docs/SYSTEM_REQUIREMENTS.md section 8.
struct ChordMusicalData {
    std::string sourceSymbol;   // display/source chord symbol, e.g. "Am7/D"
    PitchClass root = kInvalidPitchClass;   // 0..11, unused when isNoChord
    ChordQuality quality = ChordQuality::Major;
    bool hasSlashBass = false;
    PitchClass slashBass = kInvalidPitchClass;  // 0..11 when hasSlashBass
    bool isNoChord = false;  // explicit N.C.
};

// docs/SYSTEM_REQUIREMENTS.md section 9. Derived timing fields must not be
// independently hand-edited by UI code; only the mutation layer may write
// these.
struct TimingData {
    MeasureId measure = kInvalidMeasureId;
    Tick startTick = 0;         // offset within `measure`
    Tick durationTicks = 0;
    AbsoluteTick startAbsoluteTick = 0;
    AbsoluteTick endAbsoluteTick = 0;
};

// docs/SYSTEM_REQUIREMENTS.md section 7: the ordered Chord Event array is
// the harmonic timeline SSOT. Chord Boundary is derived from adjacent
// events, never stored independently.
struct ChordEvent {
    EventId id = kInvalidEventId;
    ChordMusicalData musical;
    TimingData timing;
    Provenance provenance = Provenance::Inferred;
    Verification verification = Verification::Unverified;
};

}  // namespace timeline
