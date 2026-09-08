#include "MidiInput.h"

#include <algorithm>
#include <array>
#include <cctype>

#pragma comment(lib, "winmm.lib")

namespace midi {

namespace {
std::string toLowerAscii(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(),
                    [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return s;
}
}  // namespace

bool isLikelyVirtualEndpointName(const std::string& name) {
    static constexpr std::array<const char*, 8> kVirtualMarkers = {
        "microsoft gs wavetable synth", "loopmidi",     "loopbe",
        "virtual midi",                 "vmidi",        "midi-ox",
        "rtpmidi",                      "midi through",
    };
    const std::string lower = toLowerAscii(name);
    return std::any_of(kVirtualMarkers.begin(), kVirtualMarkers.end(), [&](const char* marker) {
        return lower.find(marker) != std::string::npos;
    });
}

MidiInput::~MidiInput() {
    close();
}

std::vector<MidiDeviceInfo> MidiInput::enumeratePhysicalDevices() {
    std::vector<MidiDeviceInfo> devices = enumerateDevices();
    devices.erase(std::remove_if(devices.begin(), devices.end(),
                                  [](const MidiDeviceInfo& d) { return isLikelyVirtualEndpointName(d.name); }),
                  devices.end());
    return devices;
}

std::vector<MidiDeviceInfo> MidiInput::enumerateDevices() {
    std::vector<MidiDeviceInfo> devices;
    const UINT count = midiInGetNumDevs();
    for (UINT i = 0; i < count; ++i) {
        MIDIINCAPSW caps{};
        if (midiInGetDevCapsW(i, &caps, sizeof(caps)) == MMSYSERR_NOERROR) {
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

bool MidiInput::open(UINT deviceId) {
    close();
    state_.store(MidiConnectionState::Opening);

    MMRESULT result = midiInOpen(&handle_, deviceId, reinterpret_cast<DWORD_PTR>(&MidiInput::midiInProc),
                                  reinterpret_cast<DWORD_PTR>(this), CALLBACK_FUNCTION);
    if (result != MMSYSERR_NOERROR) {
        handle_ = nullptr;
        state_.store(MidiConnectionState::Error);
        return false;
    }

    result = midiInStart(handle_);
    if (result != MMSYSERR_NOERROR) {
        midiInClose(handle_);
        handle_ = nullptr;
        state_.store(MidiConnectionState::Error);
        return false;
    }

    state_.store(MidiConnectionState::Connected);
    return true;
}

void MidiInput::close() {
    if (handle_) {
        midiInStop(handle_);
        midiInClose(handle_);
        handle_ = nullptr;
    }
    state_.store(MidiConnectionState::Disconnected);
}

std::vector<MidiShortMessage> MidiInput::drainMessages() {
    std::lock_guard<std::mutex> lock(queueMutex_);
    std::vector<MidiShortMessage> result;
    result.swap(queue_);
    return result;
}

void CALLBACK MidiInput::midiInProc(HMIDIIN /*handle*/, UINT msg, DWORD_PTR instance,
                                     DWORD_PTR param1, DWORD_PTR param2) {
    // No C++ exception may escape this boundary (bring-up test section 11).
    try {
        auto* self = reinterpret_cast<MidiInput*>(instance);
        if (self) {
            self->handleCallback(msg, param1, param2);
        }
    } catch (...) {
        // Swallow: callback boundary must not propagate exceptions into the
        // OS multimedia driver thread.
    }
}

void MidiInput::handleCallback(UINT msg, DWORD_PTR param1, DWORD_PTR /*param2*/) {
    switch (msg) {
        case MIM_DATA: {
            MidiShortMessage decoded;
            decoded.status = static_cast<uint8_t>(param1 & 0xFF);
            decoded.data1 = static_cast<uint8_t>((param1 >> 8) & 0xFF);
            decoded.data2 = static_cast<uint8_t>((param1 >> 16) & 0xFF);
            decoded.timestampMs = static_cast<uint32_t>(timeGetTime());

            std::lock_guard<std::mutex> lock(queueMutex_);
            queue_.push_back(decoded);
            break;
        }
        case MIM_CLOSE:
            state_.store(MidiConnectionState::Disconnected);
            break;
        default:
            break;
    }
}

}  // namespace midi
