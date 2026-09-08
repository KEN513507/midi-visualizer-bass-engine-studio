#include "TestFramework.h"
#include "timeline/Timeline.h"

using namespace timeline;

namespace {

MeasuresMap buildFourMeasureMap() {
    MeasuresMap map;
    map.appendMeasure(4, 4);  // 0..15
    map.appendMeasure(4, 4);  // 16..31
    map.appendMeasure(4, 4);  // 32..47
    map.appendMeasure(4, 4);  // 48..63
    return map;
}

ChordMusicalData makeChord(const char* symbol, PitchClass root, ChordQuality quality) {
    ChordMusicalData chord;
    chord.sourceSymbol = symbol;
    chord.root = root;
    chord.quality = quality;
    return chord;
}

}  // namespace

TEST_CASE(Timeline_InitialState_IsSingleExplicitNoChord_GapFree) {
    Timeline tl(buildFourMeasureMap());

    REQUIRE_EQ(tl.events().size(), static_cast<size_t>(1));
    REQUIRE(tl.events()[0].musical.isNoChord);
    REQUIRE_EQ(tl.events()[0].timing.startAbsoluteTick, 0);
    REQUIRE_EQ(tl.events()[0].timing.endAbsoluteTick, 64);

    auto result = tl.validateTimeline();
    REQUIRE(result.ok);
}

TEST_CASE(Timeline_GapFreeInvariant_HoldsAfterSplit) {
    // GAP_GATE / OVERLAP_GATE
    Timeline tl(buildFourMeasureMap());
    REQUIRE(tl.splitChord(0, 32));

    const auto& events = tl.events();
    REQUIRE_EQ(events.size(), static_cast<size_t>(2));
    for (size_t i = 0; i + 1 < events.size(); ++i) {
        REQUIRE_EQ(events[i].timing.endAbsoluteTick, events[i + 1].timing.startAbsoluteTick);
    }
    REQUIRE(tl.validateTimeline().ok);
}

TEST_CASE(Timeline_SplitChord_RejectsBoundaryOutsideEvent) {
    Timeline tl(buildFourMeasureMap());
    // Splitting exactly at the event start/end is not a real split.
    REQUIRE_FALSE(tl.splitChord(0, 0));
    REQUIRE_FALSE(tl.splitChord(0, 64));
    REQUIRE_EQ(tl.events().size(), static_cast<size_t>(1));
}

TEST_CASE(Timeline_MoveBoundary_PreservesSharedEdge) {
    // BOUNDARY_GATE: left.end == right.start after edit.
    Timeline tl(buildFourMeasureMap());
    REQUIRE(tl.splitChord(0, 32));

    REQUIRE(tl.moveBoundary(0, 28));

    const auto& events = tl.events();
    REQUIRE_EQ(events[0].timing.endAbsoluteTick, events[1].timing.startAbsoluteTick);
    REQUIRE_EQ(events[0].timing.endAbsoluteTick, 28);
}

TEST_CASE(Timeline_MoveBoundary_RejectsZeroOrNegativeDuration) {
    // Boundary movement must preserve at least one tick duration for both
    // adjacent events.
    Timeline tl(buildFourMeasureMap());
    REQUIRE(tl.splitChord(0, 32));

    // Moving the boundary to the left event's own start collapses it to 0.
    REQUIRE_FALSE(tl.moveBoundary(0, 0));
    // Moving past the right event's end collapses the right event.
    REQUIRE_FALSE(tl.moveBoundary(0, 64));
    // Moving to exactly the right event's end collapses the right event to 0.
    REQUIRE_FALSE(tl.moveBoundary(0, 64));

    // State must remain the untouched, valid pre-mutation timeline.
    REQUIRE_EQ(tl.events()[0].timing.endAbsoluteTick, 32);
    REQUIRE(tl.validateTimeline().ok);
}

TEST_CASE(Timeline_InsertNoChord_CreatesExplicitSilentEvent) {
    // NC_GATE: silence represented by explicit N.C., not an undefined gap.
    Timeline tl(buildFourMeasureMap());
    ChordMusicalData cmaj = makeChord("C", 0, ChordQuality::Major);
    REQUIRE(tl.replaceChord(0, cmaj, Provenance::UserEdited));

    REQUIRE(tl.insertNoChord(32));

    const auto& events = tl.events();
    REQUIRE_EQ(events.size(), static_cast<size_t>(2));
    REQUIRE_FALSE(events[0].musical.isNoChord);
    REQUIRE(events[1].musical.isNoChord);
    REQUIRE_EQ(events[0].timing.endAbsoluteTick, events[1].timing.startAbsoluteTick);
}

TEST_CASE(Timeline_MergeChord_RemovesBoundaryWithoutGap) {
    Timeline tl(buildFourMeasureMap());
    REQUIRE(tl.splitChord(0, 32));
    REQUIRE_EQ(tl.events().size(), static_cast<size_t>(2));

    REQUIRE(tl.mergeChord(0));

    REQUIRE_EQ(tl.events().size(), static_cast<size_t>(1));
    REQUIRE_EQ(tl.events()[0].timing.startAbsoluteTick, 0);
    REQUIRE_EQ(tl.events()[0].timing.endAbsoluteTick, 64);
}

TEST_CASE(Timeline_DeleteChord_AbsorbsIntoNeighbor_NoGap) {
    Timeline tl(buildFourMeasureMap());
    REQUIRE(tl.splitChord(0, 16));
    REQUIRE(tl.splitChord(1, 32));
    REQUIRE_EQ(tl.events().size(), static_cast<size_t>(3));

    REQUIRE(tl.deleteChord(1));

    const auto& events = tl.events();
    REQUIRE_EQ(events.size(), static_cast<size_t>(2));
    REQUIRE_EQ(events[0].timing.endAbsoluteTick, events[1].timing.startAbsoluteTick);
    REQUIRE_EQ(events[0].timing.startAbsoluteTick, 0);
    REQUIRE_EQ(events[1].timing.endAbsoluteTick, 64);
}

TEST_CASE(Timeline_DeleteChord_RefusesToRemoveOnlyEvent) {
    Timeline tl(buildFourMeasureMap());
    REQUIRE_FALSE(tl.deleteChord(0));
    REQUIRE_EQ(tl.events().size(), static_cast<size_t>(1));
}

TEST_CASE(Timeline_ReplaceChord_ProvenanceAndVerificationAreIndependentAxes) {
    // Parser output alone must never become Verified.
    Timeline tl(buildFourMeasureMap());
    ChordMusicalData inferred = makeChord("Gm9", 7, ChordQuality::Minor);
    REQUIRE(tl.replaceChord(0, inferred, Provenance::Inferred));

    REQUIRE_EQ(tl.events()[0].provenance, Provenance::Inferred);
    REQUIRE_EQ(tl.events()[0].verification, Verification::Unverified);
}

TEST_CASE(Timeline_UserEdit_OverridesInference) {
    Timeline tl(buildFourMeasureMap());
    ChordMusicalData inferred = makeChord("Gm9", 7, ChordQuality::Minor);
    REQUIRE(tl.replaceChord(0, inferred, Provenance::Inferred));

    ChordMusicalData corrected = makeChord("Gm7", 7, ChordQuality::Minor);
    REQUIRE(tl.replaceChord(0, corrected, Provenance::UserEdited));

    REQUIRE_EQ(tl.events()[0].provenance, Provenance::UserEdited);
    REQUIRE_EQ(tl.events()[0].musical.sourceSymbol, std::string("Gm7"));
}

TEST_CASE(Timeline_SlashChord_RootAndBassAreDistinct) {
    // Am7/D: harmonic root A, bass D.
    ChordMusicalData am7SlashD = makeChord("Am7/D", 9, ChordQuality::Minor);
    am7SlashD.hasSlashBass = true;
    am7SlashD.slashBass = 2;  // D

    Timeline tl(buildFourMeasureMap());
    REQUIRE(tl.replaceChord(0, am7SlashD, Provenance::SourceExact));

    REQUIRE_EQ(tl.events()[0].musical.root, 9);
    REQUIRE_EQ(tl.events()[0].musical.slashBass, 2);
    REQUIRE(tl.events()[0].musical.hasSlashBass);
}

TEST_CASE(Timeline_NormalizeTimeline_DerivesMeasureFromMeasuresMap_NotFixedMultiplication) {
    MeasuresMap map;
    map.appendMeasure(4, 4);  // 0..15
    map.appendMeasure(2, 4);  // 16..23 (short measure)
    map.appendMeasure(4, 4);  // 24..39
    Timeline tl(map);

    REQUIRE(tl.splitChord(0, 24));

    const auto& events = tl.events();
    // The second event starts at tick 24, inside the third measure, which
    // does not begin at a multiple of a fixed 16-tick measure length.
    REQUIRE_EQ(events[1].timing.measure, map.at(2).id);
    REQUIRE_EQ(events[1].timing.startTick, 0);
}
