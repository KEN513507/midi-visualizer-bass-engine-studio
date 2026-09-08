#pragma once

#include <cstdint>

namespace timeline {

// Sixteenth note = 1 tick (see docs/SYSTEM_REQUIREMENTS.md section 5).
using Tick = int32_t;
using AbsoluteTick = int64_t;

using MeasureId = int32_t;
using EventId = int32_t;

constexpr MeasureId kInvalidMeasureId = -1;
constexpr EventId kInvalidEventId = -1;

// Harmonic root / slash bass pitch class, 0..11 (0 = C).
using PitchClass = int32_t;
constexpr PitchClass kInvalidPitchClass = -1;

constexpr Tick kTicksPerQuarterNote = 4;

enum class ChordQuality {
    Major,
    Minor,
    Dominant,
    Diminished,
    Augmented,
};

enum class Provenance {
    SourceExact,
    Inferred,
    UserEdited,
};

enum class Verification {
    Unverified,
    Verified,
    Error,
};

}  // namespace timeline
