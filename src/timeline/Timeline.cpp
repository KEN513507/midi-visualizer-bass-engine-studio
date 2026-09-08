#include "Timeline.h"

#include <algorithm>

namespace timeline {

Timeline::Timeline(MeasuresMap measuresMap) : measuresMap_(std::move(measuresMap)) {
    resetToSingleNoChord();
}

void Timeline::resetToSingleNoChord() {
    events_.clear();
    nextEventId_ = 0;

    ChordEvent event;
    event.id = nextEventId_++;
    event.musical.isNoChord = true;
    event.provenance = Provenance::SourceExact;
    event.verification = Verification::Unverified;
    event.timing.startAbsoluteTick = 0;
    event.timing.endAbsoluteTick = measuresMap_.totalLengthTicks();

    events_.push_back(event);
    normalizeTimeline();
}

void Timeline::normalizeTimeline() {
    std::sort(events_.begin(), events_.end(), [](const ChordEvent& a, const ChordEvent& b) {
        return a.timing.startAbsoluteTick < b.timing.startAbsoluteTick;
    });

    for (auto& event : events_) {
        event.timing.durationTicks =
            static_cast<Tick>(event.timing.endAbsoluteTick - event.timing.startAbsoluteTick);

        auto measure = measuresMap_.findMeasureAtAbsoluteTick(event.timing.startAbsoluteTick);
        if (measure) {
            event.timing.measure = measure->id;
            event.timing.startTick =
                static_cast<Tick>(event.timing.startAbsoluteTick - measure->startAbsoluteTick);
        } else {
            event.timing.measure = kInvalidMeasureId;
            event.timing.startTick = 0;
        }
    }
}

ValidationResult Timeline::validateTimeline() const {
    ValidationResult result;

    if (events_.empty()) {
        result.ok = false;
        result.errors.push_back("timeline has no events");
        return result;
    }

    if (events_.front().timing.startAbsoluteTick != 0) {
        result.ok = false;
        result.errors.push_back("timeline does not start at absolute tick 0");
    }

    const AbsoluteTick totalLength = measuresMap_.totalLengthTicks();
    if (events_.back().timing.endAbsoluteTick != totalLength) {
        result.ok = false;
        result.errors.push_back("timeline does not cover the full measuresMap length");
    }

    for (size_t i = 0; i < events_.size(); ++i) {
        const auto& event = events_[i];

        if (event.timing.durationTicks <= 0) {
            result.ok = false;
            result.errors.push_back("event has non-positive duration");
        }

        if (event.timing.endAbsoluteTick <= event.timing.startAbsoluteTick) {
            result.ok = false;
            result.errors.push_back("event end is not after start");
        }

        if (i + 1 < events_.size()) {
            const auto& next = events_[i + 1];
            if (event.timing.endAbsoluteTick < next.timing.startAbsoluteTick) {
                result.ok = false;
                result.errors.push_back("undefined gap between adjacent events");
            } else if (event.timing.endAbsoluteTick > next.timing.startAbsoluteTick) {
                result.ok = false;
                result.errors.push_back("overlap between adjacent events");
            }
        }
    }

    return result;
}

bool Timeline::applyGuarded(const std::function<void()>& mutate) {
    const std::vector<ChordEvent> before = events_;
    const EventId beforeNextId = nextEventId_;

    mutate();
    normalizeTimeline();

    const ValidationResult result = validateTimeline();
    if (!result.ok) {
        events_ = before;
        nextEventId_ = beforeNextId;
        return false;
    }
    return true;
}

bool Timeline::moveBoundary(size_t boundaryIndex, AbsoluteTick newAbsoluteTick) {
    if (boundaryIndex + 1 >= events_.size()) {
        return false;
    }

    return applyGuarded([&]() {
        ChordEvent& left = events_[boundaryIndex];
        ChordEvent& right = events_[boundaryIndex + 1];
        // Move the shared boundary itself; never create a hole or overlap
        // by moving one side independently.
        left.timing.endAbsoluteTick = newAbsoluteTick;
        right.timing.startAbsoluteTick = newAbsoluteTick;
    });
}

bool Timeline::replaceChord(size_t index, const ChordMusicalData& musical, Provenance provenance) {
    if (index >= events_.size()) {
        return false;
    }

    return applyGuarded([&]() {
        events_[index].musical = musical;
        events_[index].provenance = provenance;
        // Parser output alone must never become Verified (section 14);
        // any content replacement resets to Unverified for human review.
        events_[index].verification = Verification::Unverified;
    });
}

bool Timeline::splitChord(size_t index, AbsoluteTick atAbsoluteTick, bool rightIsNoChord) {
    if (index >= events_.size()) {
        return false;
    }
    if (atAbsoluteTick <= events_[index].timing.startAbsoluteTick ||
        atAbsoluteTick >= events_[index].timing.endAbsoluteTick) {
        return false;
    }

    return applyGuarded([&]() {
        ChordEvent original = events_[index];

        ChordEvent left = original;
        left.timing.endAbsoluteTick = atAbsoluteTick;

        ChordEvent right = original;
        right.id = nextEventId_++;
        right.timing.startAbsoluteTick = atAbsoluteTick;
        if (rightIsNoChord) {
            right.musical = ChordMusicalData{};
            right.musical.isNoChord = true;
            right.provenance = Provenance::UserEdited;
            right.verification = Verification::Unverified;
        }

        events_[index] = left;
        events_.insert(events_.begin() + static_cast<std::ptrdiff_t>(index) + 1, right);
    });
}

bool Timeline::insertNoChord(AbsoluteTick atAbsoluteTick) {
    for (size_t i = 0; i < events_.size(); ++i) {
        if (atAbsoluteTick > events_[i].timing.startAbsoluteTick &&
            atAbsoluteTick < events_[i].timing.endAbsoluteTick) {
            return splitChord(i, atAbsoluteTick, /*rightIsNoChord=*/true);
        }
    }
    return false;
}

bool Timeline::mergeChord(size_t index) {
    if (index + 1 >= events_.size()) {
        return false;
    }

    return applyGuarded([&]() {
        events_[index].timing.endAbsoluteTick = events_[index + 1].timing.endAbsoluteTick;
        events_[index].provenance = Provenance::UserEdited;
        events_[index].verification = Verification::Unverified;
        events_.erase(events_.begin() + static_cast<std::ptrdiff_t>(index) + 1);
    });
}

bool Timeline::deleteChord(size_t index) {
    if (index >= events_.size()) {
        return false;
    }
    if (events_.size() == 1) {
        // Deleting the only event would leave an undefined gap; forbidden.
        return false;
    }

    return applyGuarded([&]() {
        if (index + 1 < events_.size()) {
            // Absorb into the following event.
            events_[index + 1].timing.startAbsoluteTick = events_[index].timing.startAbsoluteTick;
            events_[index + 1].provenance = Provenance::UserEdited;
            events_[index + 1].verification = Verification::Unverified;
            events_.erase(events_.begin() + static_cast<std::ptrdiff_t>(index));
        } else {
            // Last event: absorb into the previous event instead.
            events_[index - 1].timing.endAbsoluteTick = events_[index].timing.endAbsoluteTick;
            events_[index - 1].provenance = Provenance::UserEdited;
            events_[index - 1].verification = Verification::Unverified;
            events_.erase(events_.begin() + static_cast<std::ptrdiff_t>(index));
        }
    });
}

}  // namespace timeline
