#pragma once

#include <string>
#include <vector>
#include <windows.h>
#include <mmsystem.h>

#include "MidiInput.h"  // MidiDeviceInfo
#include "MidiTypes.h"

namespace midi {

// Wraps the Windows winmm midiOut* API for TX and stuck-note protection
// (docs/USB_MIDI_BRINGUP_TEST.md sections 9-10).
class MidiOutput {
public:
    MidiOutput() = default;
    ~MidiOutput();

    MidiOutput(const MidiOutput&) = delete;
    MidiOutput& operator=(const MidiOutput&) = delete;

    static std::vector<MidiDeviceInfo> enumerateDevices();

    bool open(UINT deviceId);
    void close();

    bool sendNoteOn(int channel1to16, int note, int velocity);
    bool sendNoteOff(int channel1to16, int note);
    bool sendControlChange(int channel1to16, int controller, int value);

    // Sent before/during teardown so stuck notes are highly unlikely even
    // after an exception.
    bool sendAllNotesOff(int channel1to16);

private:
    bool sendShort(uint8_t status, uint8_t data1, uint8_t data2);

    HMIDIOUT handle_ = nullptr;
};

}  // namespace midi
