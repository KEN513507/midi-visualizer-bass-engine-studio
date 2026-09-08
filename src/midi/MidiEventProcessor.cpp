#include "MidiEventProcessor.h"

namespace midi {

MidiEventProcessor::ProcessedEvents MidiEventProcessor::process(
    const std::vector<MidiShortMessage>& messages) const {
    ProcessedEvents result;

    for (const auto& msg : messages) {
        const MidiCommand command = decodeCommand(msg);
        switch (command) {
            case MidiCommand::NoteOn: {
                NoteEvent event{msg.channel(), msg.data1, msg.data2, msg.timestampMs};
                result.noteOn.push_back(event);
                break;
            }
            case MidiCommand::NoteOff: {
                // decodeCommand already folds velocity-zero NoteOn into
                // NoteOff; the reported velocity is preserved for logging.
                NoteEvent event{msg.channel(), msg.data1, msg.data2, msg.timestampMs};
                result.noteOff.push_back(event);
                break;
            }
            case MidiCommand::ControlChange: {
                if (msg.data1 == kControllerSustain) {
                    SustainEvent event{msg.channel(), isSustainOn(msg.data2), msg.timestampMs};
                    result.sustain.push_back(event);
                }
                break;
            }
            default:
                break;
        }
    }

    return result;
}

}  // namespace midi
