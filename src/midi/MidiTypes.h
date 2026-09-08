#pragma once

#include <cstdint>
#include <string>

// docs/USB_MIDI_BRINGUP_TEST.md: Windows-native MIDI API, physical USB
// device path only. This header holds pure, hardware-independent decoding
// so it is unit-testable without a physical device attached.
namespace midi {

enum class MidiConnectionState {
    Disconnected,
    Enumerating,
    Opening,
    Connected,
    Error,
};

enum class MidiCommand {
    NoteOff,
    NoteOn,
    ControlChange,
    PitchBend,
    Other,
};

struct MidiShortMessage {
    uint32_t timestampMs = 0;
    uint8_t status = 0;
    uint8_t data1 = 0;
    uint8_t data2 = 0;

    int channel() const { return (status & 0x0F) + 1; }
    uint8_t commandByte() const { return status & 0xF0; }
};

inline MidiCommand decodeCommand(const MidiShortMessage& msg) {
    switch (msg.commandByte()) {
        case 0x80:
            return MidiCommand::NoteOff;
        case 0x90:
            // A Note On with velocity zero must be normalized to Note Off
            // behavior (docs/USB_MIDI_BRINGUP_TEST.md section 5).
            return msg.data2 == 0 ? MidiCommand::NoteOff : MidiCommand::NoteOn;
        case 0xB0:
            return MidiCommand::ControlChange;
        case 0xE0:
            return MidiCommand::PitchBend;
        default:
            return MidiCommand::Other;
    }
}

// Human-facing octave naming is fixed: MIDI 60 = C4 (docs/SYSTEM_REQUIREMENTS.md
// section 4). MIDI 0 = C-1 under this convention (60 - 12*5 = 0 => C(4-5) = C-1).
inline std::string noteName(int midiNote) {
    static const char* kNames[12] = {"C", "C#", "D", "D#", "E", "F",
                                      "F#", "G", "G#", "A", "A#", "B"};
    if (midiNote < 0 || midiNote > 127) {
        return "?";
    }
    const int pitchClass = midiNote % 12;
    const int octave = (midiNote / 12) - 1;
    return std::string(kNames[pitchClass]) + std::to_string(octave);
}

// CC64 sustain interpretation (docs/USB_MIDI_BRINGUP_TEST.md section 8).
inline bool isSustainOn(uint8_t cc64Value) {
    return cc64Value >= 64;
}

constexpr uint8_t kControllerSustain = 64;
constexpr uint8_t kControllerAllNotesOff = 123;

}  // namespace midi
