// USB-MIDI console bring-up harness (docs/USB_MIDI_BRINGUP_TEST.md).
// Proves the physical USB-MIDI path independently before any DX11/timeline
// integration (bring-up isolation rule, section 15).
#include <algorithm>
#include <atomic>
#include <chrono>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <thread>
#include <windows.h>

#include "midi/MidiInput.h"
#include "midi/MidiOutput.h"

namespace {

std::atomic<bool> g_running{true};

std::string toLower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return s;
}

// Picks the output device most likely to be the same physical unit as the
// opened input device, rather than defaulting to a software synth like
// "Microsoft GS Wavetable Synth" for the PC -> MIDI OUT test.
size_t selectMatchingOutput(const std::vector<midi::MidiDeviceInfo>& outputs, const std::string& inputName) {
    const std::string needle = toLower(inputName);
    for (size_t i = 0; i < outputs.size(); ++i) {
        if (toLower(outputs[i].name).find(needle) != std::string::npos) {
            return i;
        }
    }
    for (size_t i = 0; i < outputs.size(); ++i) {
        if (toLower(outputs[i].name).find("wavetable") == std::string::npos) {
            return i;
        }
    }
    return 0;
}

BOOL WINAPI consoleCtrlHandler(DWORD ctrlType) {
    switch (ctrlType) {
        case CTRL_C_EVENT:
        case CTRL_BREAK_EVENT:
        case CTRL_CLOSE_EVENT:
            g_running.store(false);
            return TRUE;
        default:
            return FALSE;
    }
}

void logMessage(const midi::MidiShortMessage& msg) {
    using midi::MidiCommand;
    const MidiCommand command = midi::decodeCommand(msg);
    const int channel = msg.channel();

    switch (command) {
        case MidiCommand::NoteOn:
            std::printf("[MIDI][RX] ts=%ums ch=%d NOTE_ON  note=%d %s vel=%d\n", msg.timestampMs,
                        channel, msg.data1, midi::noteName(msg.data1).c_str(), msg.data2);
            break;
        case MidiCommand::NoteOff:
            std::printf("[MIDI][RX] ts=%ums ch=%d NOTE_OFF note=%d %s vel=%d\n", msg.timestampMs,
                        channel, msg.data1, midi::noteName(msg.data1).c_str(), msg.data2);
            break;
        case MidiCommand::ControlChange:
            if (msg.data1 == midi::kControllerSustain) {
                std::printf("[MIDI][RX] ts=%ums ch=%d CC controller=64 value=%d SUSTAIN=%s\n",
                            msg.timestampMs, channel, msg.data2,
                            midi::isSustainOn(msg.data2) ? "ON" : "OFF");
            } else {
                std::printf("[MIDI][RX] ts=%ums ch=%d CC controller=%d value=%d\n", msg.timestampMs,
                            channel, msg.data1, msg.data2);
            }
            break;
        case MidiCommand::PitchBend:
            std::printf("[MIDI][RX] ts=%ums ch=%d PITCH_BEND lsb=%d msb=%d\n", msg.timestampMs,
                        channel, msg.data1, msg.data2);
            break;
        case MidiCommand::Other:
            if (msg.status == 0xF8 || msg.status == 0xFE) {
                // System real-time timing clock (0xF8) / active sensing
                // (0xFE): high-frequency, not part of the minimum bring-up
                // decode set. Suppress to keep the log readable; still
                // received/handled, just not logged per-tick.
                break;
            }
            std::printf("[MIDI][RX] ts=%ums ch=%d OTHER status=0x%02X data1=%d data2=%d\n",
                        msg.timestampMs, channel, msg.status, msg.data1, msg.data2);
            break;
    }
}

}  // namespace

int main(int argc, char** argv) {
    SetConsoleCtrlHandler(consoleCtrlHandler, TRUE);

    // Optional: auto-stop after N seconds, for unattended evidence capture.
    // With no argument the harness runs until Ctrl+C, as normal interactive
    // bring-up use expects.
    double autoStopSeconds = -1.0;
    if (argc > 1) {
        autoStopSeconds = std::atof(argv[1]);
    }

    // MIDI-01: enumerate input/output devices.
    auto inputs = midi::MidiInput::enumerateDevices();
    auto outputs = midi::MidiOutput::enumerateDevices();

    for (const auto& dev : inputs) {
        std::printf("[MIDI][ENUM][IN ][%u] %s\n", dev.id, dev.name.c_str());
    }
    for (const auto& dev : outputs) {
        std::printf("[MIDI][ENUM][OUT][%u] %s\n", dev.id, dev.name.c_str());
    }

    if (inputs.empty()) {
        std::printf("[MIDI][WARN] no physical MIDI input devices enumerated\n");
    }
    if (outputs.empty()) {
        std::printf("[MIDI][WARN] no physical MIDI output devices enumerated\n");
    }

    midi::MidiInput midiIn;
    midi::MidiOutput midiOut;

    bool inOpen = false;
    bool outOpen = false;

    if (!inputs.empty()) {
        inOpen = midiIn.open(inputs.front().id);
        std::printf("[MIDI][OPEN][IN ] id=%u %s\n", inputs.front().id, inOpen ? "OK" : "FAILED");
        if (!inOpen) {
            std::printf("[MIDI][ERROR] midi input open failed id=%u\n", inputs.front().id);
        }
    }

    if (!outputs.empty()) {
        const std::string inputName = inputs.empty() ? std::string() : inputs.front().name;
        const size_t outIndex = selectMatchingOutput(outputs, inputName);
        const auto& chosen = outputs[outIndex];
        outOpen = midiOut.open(chosen.id);
        std::printf("[MIDI][OPEN][OUT] id=%u %s %s\n", chosen.id, chosen.name.c_str(), outOpen ? "OK" : "FAILED");
        if (!outOpen) {
            std::printf("[MIDI][ERROR] midi output open failed id=%u\n", chosen.id);
        }
    }

    if (outOpen) {
        // MIDI-05: conservative PC -> MIDI OUT test note, C4 / MIDI 60 / vel 96.
        std::printf("[MIDI][TX] ch=1 NOTE_ON  note=60 C4 vel=96\n");
        midiOut.sendNoteOn(1, 60, 96);
        Sleep(200);
        std::printf("[MIDI][TX] ch=1 NOTE_OFF note=60 C4 vel=0\n");
        midiOut.sendNoteOff(1, 60);
    }

    if (!inOpen) {
        // Nothing further to observe without a physical input device; this
        // run still proves enumeration/output behavior deterministically.
        std::printf("[MIDI][CLOSE] no input device open, exiting bring-up loop\n");
        return inputs.empty() ? 0 : 1;
    }

    std::printf("[MIDI][RX] listening for physical NoteOn/NoteOff/CC64 (Ctrl+C to stop)...\n");

    const auto startTime = std::chrono::steady_clock::now();
    while (g_running.load()) {
        for (const auto& msg : midiIn.drainMessages()) {
            logMessage(msg);
        }
        if (autoStopSeconds >= 0.0) {
            const auto elapsed = std::chrono::duration<double>(std::chrono::steady_clock::now() - startTime).count();
            if (elapsed >= autoStopSeconds) {
                break;
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }

    // MIDI-06 / section 10: stuck-note protection at shutdown.
    if (outOpen) {
        for (int ch = 1; ch <= 16; ++ch) {
            midiOut.sendAllNotesOff(ch);
        }
    }

    midiIn.close();
    midiOut.close();
    std::printf("[MIDI][CLOSE] shutdown complete\n");
    return 0;
}
