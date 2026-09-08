#pragma once

#include <vector>

#include "timeline/MeasuresMap.h"
#include "timeline/Types.h"

namespace bass {

// docs/SYSTEM_REQUIREMENTS.md section 16: bass pattern timing is separate
// from chord correctness. Pattern logic must respect actual measure length
// through metric data, never a fixed-length assumption.
enum class BassPatternType {
    EightBeatCityPop,
    JazzWalking,
    LatinBossa,
    RootHalfNote,
};

// Returns candidate trigger offsets (ticks relative to the start of
// `measure`), derived proportionally from measure.lengthTicks so 2/4, 3/4,
// 4/4 and 5/4 all scale correctly instead of assuming a fixed 4/4 length.
// For a standard 4/4 measure (16 ticks) this reproduces the exact tick
// lists given in the spec.
std::vector<timeline::Tick> patternTriggerOffsets(BassPatternType type,
                                                    const timeline::MeasureMetric& measure);

}  // namespace bass
