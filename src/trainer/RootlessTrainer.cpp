#include "RootlessTrainer.h"

namespace trainer {

RootlessTrainer::RootlessTrainer() {
    monitoredOctaves_.fill(false);
}

void RootlessTrainer::setMonitoredOctave(int octave, bool enabled) {
    if (octave < kMinOctave || octave > kMaxOctave) {
        return;
    }
    monitoredOctaves_[static_cast<size_t>(octave - kMinOctave)] = enabled;
}

bool RootlessTrainer::isOctaveMonitored(int octave) const {
    if (octave < kMinOctave || octave > kMaxOctave) {
        return false;
    }
    return monitoredOctaves_[static_cast<size_t>(octave - kMinOctave)];
}

void RootlessTrainer::applyOct3To5Preset() {
    clearAllMonitoredOctaves();
    setMonitoredOctave(3, true);
    setMonitoredOctave(4, true);
    setMonitoredOctave(5, true);
}

void RootlessTrainer::clearAllMonitoredOctaves() {
    monitoredOctaves_.fill(false);
}

void RootlessTrainer::setCurrentChord(timeline::PitchClass root, bool isNoChord) {
    currentRoot_ = root;
    currentIsNoChord_ = isNoChord;
}

int RootlessTrainer::octaveOf(int midiNote) {
    // MIDI 60 = C4 (docs/SYSTEM_REQUIREMENTS.md section 4).
    return (midiNote / 12) - 1;
}

timeline::PitchClass RootlessTrainer::pitchClassOf(int midiNote) {
    return midiNote % 12;
}

std::optional<RootlessTrainer::Violation> RootlessTrainer::onNoteOn(int midiNote, int velocity) {
    if (currentIsNoChord_ || currentRoot_ == timeline::kInvalidPitchClass) {
        return std::nullopt;
    }

    const int octave = octaveOf(midiNote);
    if (!isOctaveMonitored(octave)) {
        return std::nullopt;
    }

    const timeline::PitchClass pitchClass = pitchClassOf(midiNote);
    if (pitchClass != currentRoot_) {
        return std::nullopt;
    }

    Violation violation;
    violation.midiNote = midiNote;
    violation.velocity = velocity;
    violation.offendingPitchClass = pitchClass;
    violation.octave = octave;
    return violation;
}

}  // namespace trainer
