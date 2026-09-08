#pragma once

#include <optional>
#include <vector>

#include "Types.h"

namespace timeline {

// measuresMap: the metric SSOT (docs/SYSTEM_REQUIREMENTS.md section 6).
// V1 supports quarter-note denominator meters only (section 5).
struct MeasureMetric {
    MeasureId id = kInvalidMeasureId;
    int32_t numerator = 4;
    int32_t denominator = 4;
    Tick lengthTicks = 0;
    AbsoluteTick startAbsoluteTick = 0;
    AbsoluteTick endAbsoluteTick = 0;
};

// Never derive musical position using a fixed measure length. All lookups
// walk measuresMap; no measureIndex * FIXED_TICKS_PER_MEASURE arithmetic.
class MeasuresMap {
public:
    // Appends a new measure after the current end of the map. Only
    // quarter-note denominators are accepted in V1.
    MeasureId appendMeasure(int32_t numerator, int32_t denominator);

    void clear();

    size_t size() const { return measures_.size(); }
    bool empty() const { return measures_.empty(); }

    const MeasureMetric& at(size_t index) const { return measures_.at(index); }
    const std::vector<MeasureMetric>& measures() const { return measures_; }

    AbsoluteTick totalLengthTicks() const;

    // Returns the measure containing absoluteTick, or nullopt if out of range.
    std::optional<MeasureMetric> findMeasureAtAbsoluteTick(AbsoluteTick absoluteTick) const;

    std::optional<MeasureMetric> getMeasureById(MeasureId id) const;

    AbsoluteTick getMeasureStart(MeasureId id) const;
    AbsoluteTick getMeasureEnd(MeasureId id) const;

    std::optional<MeasureMetric> getPreviousMeasure(MeasureId id) const;
    std::optional<MeasureMetric> getNextMeasure(MeasureId id) const;

private:
    std::vector<MeasureMetric> measures_;
    MeasureId nextId_ = 0;
};

}  // namespace timeline
