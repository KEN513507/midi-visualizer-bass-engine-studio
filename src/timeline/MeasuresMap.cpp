#include "MeasuresMap.h"

#include <stdexcept>

namespace timeline {

MeasureId MeasuresMap::appendMeasure(int32_t numerator, int32_t denominator) {
    if (numerator <= 0) {
        throw std::invalid_argument("MeasuresMap::appendMeasure: numerator must be positive");
    }
    if (denominator != 4) {
        // V1 uses quarter-note denominator meters only (section 5).
        throw std::invalid_argument("MeasuresMap::appendMeasure: only quarter-note denominator meters are supported in V1");
    }

    MeasureMetric metric;
    metric.id = nextId_++;
    metric.numerator = numerator;
    metric.denominator = denominator;
    metric.lengthTicks = numerator * kTicksPerQuarterNote;
    metric.startAbsoluteTick = totalLengthTicks();
    metric.endAbsoluteTick = metric.startAbsoluteTick + metric.lengthTicks;

    measures_.push_back(metric);
    return metric.id;
}

void MeasuresMap::clear() {
    measures_.clear();
    nextId_ = 0;
}

AbsoluteTick MeasuresMap::totalLengthTicks() const {
    if (measures_.empty()) {
        return 0;
    }
    return measures_.back().endAbsoluteTick;
}

std::optional<MeasureMetric> MeasuresMap::findMeasureAtAbsoluteTick(AbsoluteTick absoluteTick) const {
    for (const auto& measure : measures_) {
        if (absoluteTick >= measure.startAbsoluteTick && absoluteTick < measure.endAbsoluteTick) {
            return measure;
        }
    }
    return std::nullopt;
}

std::optional<MeasureMetric> MeasuresMap::getMeasureById(MeasureId id) const {
    for (const auto& measure : measures_) {
        if (measure.id == id) {
            return measure;
        }
    }
    return std::nullopt;
}

AbsoluteTick MeasuresMap::getMeasureStart(MeasureId id) const {
    auto measure = getMeasureById(id);
    if (!measure) {
        throw std::out_of_range("MeasuresMap::getMeasureStart: unknown measure id");
    }
    return measure->startAbsoluteTick;
}

AbsoluteTick MeasuresMap::getMeasureEnd(MeasureId id) const {
    auto measure = getMeasureById(id);
    if (!measure) {
        throw std::out_of_range("MeasuresMap::getMeasureEnd: unknown measure id");
    }
    return measure->endAbsoluteTick;
}

std::optional<MeasureMetric> MeasuresMap::getPreviousMeasure(MeasureId id) const {
    for (size_t i = 0; i < measures_.size(); ++i) {
        if (measures_[i].id == id) {
            if (i == 0) {
                return std::nullopt;
            }
            return measures_[i - 1];
        }
    }
    return std::nullopt;
}

std::optional<MeasureMetric> MeasuresMap::getNextMeasure(MeasureId id) const {
    for (size_t i = 0; i < measures_.size(); ++i) {
        if (measures_[i].id == id) {
            if (i + 1 >= measures_.size()) {
                return std::nullopt;
            }
            return measures_[i + 1];
        }
    }
    return std::nullopt;
}

}  // namespace timeline
