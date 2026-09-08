#include "MidiOutput.h"

#pragma comment(lib, "winmm.lib")

namespace midi {

MidiOutput::~MidiOutput() {
    close();
}

std::vector<MidiDeviceInfo> MidiOutput::enumerateDevices() {
    std::vector<MidiDeviceInfo> devices;
    const UINT count = midiOutGetNumDevs();
    for (UINT i = 0; i < count; ++i) {
        MIDIOUTCAPSW caps{};
        if (midiOutGetDevCapsW(i, &caps, sizeof(caps)) == MMSYSERR_NOERROR) {
            int len = WideCharToMultiByte(CP_UTF8, 0, caps.szPname, -1, nullptr, 0, nullptr, nullptr);
            std::string name;
            if (len > 0) {
                name.resize(static_cast<size_t>(len) - 1);
                WideCharToMultiByte(CP_UTF8, 0, caps.szPname, -1, name.data(), len, nullptr, nullptr);
            }
            devices.push_back(MidiDeviceInfo{i, name});
        }
    }
    return devices;
}

bool MidiOutput::open(UINT deviceId) {
    close();
    MMRESULT result = midiOutOpen(&handle_, deviceId, 0, 0, CALLBACK_NULL);
    if (result != MMSYSERR_NOERROR) {
        handle_ = nullptr;
        return false;
    }
    return true;
}

void MidiOutput::close() {
    if (handle_) {
        midiOutClose(handle_);
        handle_ = nullptr;
    }
}

bool MidiOutput::sendShort(uint8_t status, uint8_t data1, uint8_t data2) {
    if (!handle_) {
        return false;
    }
    const DWORD msg = static_cast<DWORD>(status) | (static_cast<DWORD>(data1) << 8) |
                       (static_cast<DWORD>(data2) << 16);
    return midiOutShortMsg(handle_, msg) == MMSYSERR_NOERROR;
}

bool MidiOutput::sendNoteOn(int channel1to16, int note, int velocity) {
    const uint8_t status = static_cast<uint8_t>(0x90 | ((channel1to16 - 1) & 0x0F));
    return sendShort(status, static_cast<uint8_t>(note), static_cast<uint8_t>(velocity));
}

bool MidiOutput::sendNoteOff(int channel1to16, int note) {
    const uint8_t status = static_cast<uint8_t>(0x80 | ((channel1to16 - 1) & 0x0F));
    return sendShort(status, static_cast<uint8_t>(note), 0);
}

bool MidiOutput::sendControlChange(int channel1to16, int controller, int value) {
    const uint8_t status = static_cast<uint8_t>(0xB0 | ((channel1to16 - 1) & 0x0F));
    return sendShort(status, static_cast<uint8_t>(controller), static_cast<uint8_t>(value));
}

bool MidiOutput::sendAllNotesOff(int channel1to16) {
    return sendControlChange(channel1to16, kControllerAllNotesOff, 0);
}

}  // namespace midi
