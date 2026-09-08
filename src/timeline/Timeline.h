#pragma once

#include <functional>
#include <string>
#include <vector>

#include "ChordEvent.h"
#include "MeasuresMap.h"
#include "Types.h"

namespace timeline {

struct ValidationResult {
    bool ok = true;
    std::vector<std::string> errors;
};

// docs/SYSTEM_REQUIREMENTS.md section 13: all timeline modifications go
// through this mutation layer. Required processing sequence:
//   User operation -> Mutation API -> normalizeTimeline() -> validateTimeline()
//   -> publish playback snapshot -> render updated state
// An operation that would leave the timeline invalid is rejected and the
// prior valid state is preserved (invalid state must not reach playback).
class Timeline {
public:
    explicit Timeline(MeasuresMap measuresMap);

    // Resets the event array to a single explicit N.C. event spanning the
    // entire measuresMap. There is no "undefined gap" state in this model.
    void resetToSingleNoChord();

    const std::vector<ChordEvent>& events() const { return events_; }
    const MeasuresMap& measuresMap() const { return measuresMap_; }

    // Recomputes derived fields (measure, startTick, durationTicks) from
    // startAbsoluteTick/endAbsoluteTick and sorts events by start time.
    void normalizeTimeline();

    // Checks gap-free / non-overlap / positive-duration / full-coverage
    // invariants. Does not mutate state.
    ValidationResult validateTimeline() const;

    // --- Mutation API (docs/SYSTEM_REQUIREMENTS.md section 13) ---

    // Moves the shared boundary between events[boundaryIndex] and
    // events[boundaryIndex + 1] to newAbsoluteTick. Both adjacent events
    // must retain at least one tick of duration.
    bool moveBoundary(size_t boundaryIndex, AbsoluteTick newAbsoluteTick);

    // Replaces only the musical content of an event; timing is untouched.
    bool replaceChord(size_t index, const ChordMusicalData& musical, Provenance provenance);

    // Splits the event at `index` into two events at `atAbsoluteTick`.
    // Both resulting events keep the original musical data unless
    // `rightIsNoChord` is set, in which case the right-hand event becomes
    // an explicit N.C. event. Used to implement "insert N.C." editing.
    bool splitChord(size_t index, AbsoluteTick atAbsoluteTick, bool rightIsNoChord = false);

    // Convenience wrapper: splits the event containing atAbsoluteTick and
    // marks the resulting right-hand segment as explicit N.C.
    bool insertNoChord(AbsoluteTick atAbsoluteTick);

    // Merges event at `index` with the following event into a single event.
    // The merged event keeps the left event's musical data.
    bool mergeChord(size_t index);

    // Removes the event at `index` by absorbing its duration into a
    // neighbor (the following event if one exists, otherwise the previous
    // one), so no gap is ever created.
    bool deleteChord(size_t index);

private:
    MeasuresMap measuresMap_;
    std::vector<ChordEvent> events_;
    EventId nextEventId_ = 0;

    // Applies `mutate`, then normalizes and validates; rolls back to the
    // pre-mutation state if validation fails. Returns true on success.
    bool applyGuarded(const std::function<void()>& mutate);
};

}  // namespace timeline
