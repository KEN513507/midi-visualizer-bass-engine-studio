#include <stdexcept>

#include "TestFramework.h"
#include "timeline/MeasuresMap.h"

using namespace timeline;

namespace {

MeasuresMap buildDocExampleMap() {
    // docs/SYSTEM_REQUIREMENTS.md section 6 example:
    // M1 4/4 =  0..15
    // M2 4/4 = 16..31
    // M3 2/4 = 32..39
    // M4 4/4 = 40..55
    MeasuresMap map;
    map.appendMeasure(4, 4);
    map.appendMeasure(4, 4);
    map.appendMeasure(2, 4);
    map.appendMeasure(4, 4);
    return map;
}

}  // namespace

TEST_CASE(MeasuresMap_VariableMeasureLength_MatchesDocExample) {
    MeasuresMap map = buildDocExampleMap();

    REQUIRE_EQ(map.size(), static_cast<size_t>(4));

    REQUIRE_EQ(map.at(0).startAbsoluteTick, 0);
    REQUIRE_EQ(map.at(0).endAbsoluteTick, 16);

    REQUIRE_EQ(map.at(1).startAbsoluteTick, 16);
    REQUIRE_EQ(map.at(1).endAbsoluteTick, 32);

    REQUIRE_EQ(map.at(2).startAbsoluteTick, 32);
    REQUIRE_EQ(map.at(2).endAbsoluteTick, 40);

    // M4 begins at absolute tick 40, never at a fixed-multiply result
    // (4 * 16 would incorrectly give 64).
    REQUIRE_EQ(map.at(3).startAbsoluteTick, 40);
    REQUIRE_EQ(map.at(3).endAbsoluteTick, 56);
}

TEST_CASE(MeasuresMap_4_4_to_2_4_to_4_4_RemainsExact) {
    // METRIC_GATE: 4/4 -> 2/4 -> 4/4 remains exact.
    MeasuresMap map;
    map.appendMeasure(4, 4);  // 0..15
    map.appendMeasure(2, 4);  // 16..23
    map.appendMeasure(4, 4);  // 24..39

    REQUIRE_EQ(map.at(0).endAbsoluteTick, 16);
    REQUIRE_EQ(map.at(1).startAbsoluteTick, 16);
    REQUIRE_EQ(map.at(1).endAbsoluteTick, 24);
    REQUIRE_EQ(map.at(2).startAbsoluteTick, 24);
    REQUIRE_EQ(map.at(2).endAbsoluteTick, 40);
}

TEST_CASE(MeasuresMap_FindMeasureAtAbsoluteTick_NoFixedMultiplication) {
    MeasuresMap map = buildDocExampleMap();

    // Tick 40 falls in M4, which does not start at a multiple of a fixed
    // measure length (M3 was only 2/4).
    auto found = map.findMeasureAtAbsoluteTick(40);
    REQUIRE(found.has_value());
    REQUIRE_EQ(found->id, map.at(3).id);

    auto boundary = map.findMeasureAtAbsoluteTick(39);
    REQUIRE(boundary.has_value());
    REQUIRE_EQ(boundary->id, map.at(2).id);
}

TEST_CASE(MeasuresMap_PreviousAndNextMeasure) {
    MeasuresMap map = buildDocExampleMap();

    auto prev = map.getPreviousMeasure(map.at(2).id);
    REQUIRE(prev.has_value());
    REQUIRE_EQ(prev->id, map.at(1).id);

    auto next = map.getNextMeasure(map.at(2).id);
    REQUIRE(next.has_value());
    REQUIRE_EQ(next->id, map.at(3).id);

    REQUIRE_FALSE(map.getPreviousMeasure(map.at(0).id).has_value());
    REQUIRE_FALSE(map.getNextMeasure(map.at(3).id).has_value());
}

TEST_CASE(MeasuresMap_RejectsNonQuarterDenominator) {
    MeasuresMap map;
    bool threw = false;
    try {
        map.appendMeasure(6, 8);
    } catch (const std::invalid_argument&) {
        threw = true;
    }
    REQUIRE(threw);
}
