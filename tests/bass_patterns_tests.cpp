#include "TestFramework.h"
#include "bass/BassPatterns.h"

using namespace bass;
using namespace timeline;

namespace {

MeasureMetric make44Measure() {
    MeasuresMap map;
    map.appendMeasure(4, 4);
    return map.at(0);
}

MeasureMetric make24Measure() {
    MeasuresMap map;
    map.appendMeasure(2, 4);
    return map.at(0);
}

}  // namespace

TEST_CASE(BassPattern_EightBeatCityPop_MatchesDocExample_4_4) {
    auto offsets = patternTriggerOffsets(BassPatternType::EightBeatCityPop, make44Measure());
    std::vector<Tick> expected = {0, 2, 4, 6, 8, 10, 12, 14};
    REQUIRE_EQ(offsets.size(), expected.size());
    for (size_t i = 0; i < expected.size(); ++i) {
        REQUIRE_EQ(offsets[i], expected[i]);
    }
}

TEST_CASE(BassPattern_JazzWalking_MatchesDocExample_4_4) {
    auto offsets = patternTriggerOffsets(BassPatternType::JazzWalking, make44Measure());
    std::vector<Tick> expected = {0, 4, 8, 12};
    REQUIRE_EQ(offsets.size(), expected.size());
    for (size_t i = 0; i < expected.size(); ++i) {
        REQUIRE_EQ(offsets[i], expected[i]);
    }
}

TEST_CASE(BassPattern_LatinBossa_MatchesDocExample_4_4) {
    auto offsets = patternTriggerOffsets(BassPatternType::LatinBossa, make44Measure());
    REQUIRE_EQ(offsets.size(), static_cast<size_t>(2));
    REQUIRE_EQ(offsets[0], 0);
    REQUIRE_EQ(offsets[1], 10);
}

TEST_CASE(BassPattern_RootHalfNote_MatchesDocExample_4_4) {
    auto offsets = patternTriggerOffsets(BassPatternType::RootHalfNote, make44Measure());
    REQUIRE_EQ(offsets.size(), static_cast<size_t>(2));
    REQUIRE_EQ(offsets[0], 0);
    REQUIRE_EQ(offsets[1], 8);
}

TEST_CASE(BassPattern_RespectsActualMeasureLength_NotFixed4_4) {
    // 2/4 measure is 8 ticks, not 16. Patterns must scale, not assume 4/4.
    auto eightBeat = patternTriggerOffsets(BassPatternType::EightBeatCityPop, make24Measure());
    std::vector<Tick> expectedEightBeat = {0, 2, 4, 6};
    REQUIRE_EQ(eightBeat.size(), expectedEightBeat.size());
    for (size_t i = 0; i < expectedEightBeat.size(); ++i) {
        REQUIRE_EQ(eightBeat[i], expectedEightBeat[i]);
    }

    auto rootHalf = patternTriggerOffsets(BassPatternType::RootHalfNote, make24Measure());
    REQUIRE_EQ(rootHalf.size(), static_cast<size_t>(2));
    REQUIRE_EQ(rootHalf[0], 0);
    REQUIRE_EQ(rootHalf[1], 4);  // half of 8 ticks, not half of 16
}

TEST_CASE(BassPattern_AllOffsets_WithinMeasureBounds) {
    for (auto type : {BassPatternType::EightBeatCityPop, BassPatternType::JazzWalking,
                       BassPatternType::LatinBossa, BassPatternType::RootHalfNote}) {
        auto offsets = patternTriggerOffsets(type, make44Measure());
        for (Tick t : offsets) {
            REQUIRE(t >= 0);
            REQUIRE(t < 16);
        }
    }
}
