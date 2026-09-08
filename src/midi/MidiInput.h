#pragma once

#include <atomic>
#include <mutex>
#include <string>
#include <vector>
#include <windows.h>
#include <mmsystem.h>

#include "MidiTypes.h"

namespace midi {

struct MidiDeviceInfo {
    UINT id = 0;
    std::string name;
};

// WinMM exposes no reliable driver-type flag for input devices, so physical
// vs. virtual/software endpoints are distinguished by name heuristic
// (requested bring-up behavior: reject/ignore virtual endpoints where
// feasible). False negatives (an unrecognized virtual port) are possible;
// this is a best-effort filter, not a hardware guarantee.
bool isLikelyVirtualEndpointName(const std::string& name);

// Wraps the Windows winmm midiIn* API. Physical USB-MIDI device path only
// (docs/USB_MIDI_BRINGUP_TEST.md). The driver callback must never let a C++
// exception escape (section 11), so it only enqueues raw messages; all
// interpretation/logging happens on the caller's thread via drainMessages().
class MidiInput {
public:
    MidiInput() = default;
    ~MidiInput();

    MidiInput(const MidiInput&) = delete;
    MidiInput& operator=(const MidiInput&) = delete;

    static std::vector<MidiDeviceInfo> enumerateDevices();

    // Same as enumerateDevices(), minus entries matching
    // isLikelyVirtualEndpointName(). Preferred for automatic device
    // selection; enumerateDevices() remains available for diagnostics.
    static std::vector<MidiDeviceInfo> enumeratePhysicalDevices();

    bool open(UINT deviceId);
    void close();

    MidiConnectionState state() const { return state_.load(); }

    // Returns and clears all messages received since the last call.
    std::vector<MidiShortMessage> drainMessages();

private:
    static void CALLBACK midiInProc(HMIDIIN handle, UINT msg, DWORD_PTR instance,
                                     DWORD_PTR param1, DWORD_PTR param2);
    void handleCallback(UINT msg, DWORD_PTR param1, DWORD_PTR param2);

    HMIDIIN handle_ = nullptr;
    std::mutex queueMutex_;
    std::vector<MidiShortMessage> queue_;
    std::atomic<MidiConnectionState> state_{MidiConnectionState::Disconnected};
};

}  // namespace midi
