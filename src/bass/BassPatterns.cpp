#include "BassPatterns.h"

#include <cmath>

namespace bass {

using timeline::MeasureMetric;
using timeline::Tick;

std::vector<Tick> patternTriggerOffsets(BassPatternType type, const MeasureMetric& measure) {
    std::vector<Tick> offsets;
    const Tick length = measure.lengthTicks;
    if (length <= 0) {
        return offsets;
    }

    switch (type) {
        case BassPatternType::EightBeatCityPop:
            // Eighth-note triggers: every 2 ticks (4/4 -> 0,2,4,...,14).
            for (Tick t = 0; t < length; t += 2) {
                offsets.push_back(t);
            }
            break;

        case BassPatternType::JazzWalking:
            // Quarter-note triggers: every 4 ticks (4/4 -> 0,4,8,12).
            for (Tick t = 0; t < length; t += timeline::kTicksPerQuarterNote) {
                offsets.push_back(t);
            }
            break;

        case BassPatternType::RootHalfNote:
            // Downbeat plus halfway point (4/4 -> 0,8).
            offsets.push_back(0);
            if (length > 1) {
                offsets.push_back(length / 2);
            }
            break;

        case BassPatternType::LatinBossa: {
            // Downbeat plus the syncopated "and of 3" push (4/4: tick 10 of 16,
            // i.e. 0.625 of the measure), scaled proportionally to actual
            // measure length rather than hard-coded to 4/4.
            offsets.push_back(0);
            const Tick syncopated =
                static_cast<Tick>(std::lround(static_cast<double>(length) * 10.0 / 16.0));
            if (syncopated > 0 && syncopated < length) {
                offsets.push_back(syncopated);
            }
            break;
        }
    }

    return offsets;
}

}  // namespace bass
