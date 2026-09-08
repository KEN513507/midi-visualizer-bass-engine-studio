#include <windows.h>

#include <array>
#include <cstdio>
#include <string>

#include "app/KeyboardState.h"
#include "app/PianoLayout.h"
#include "app/RendererDX11.h"
#include "app/Window.h"
#include "midi/MidiEventProcessor.h"
#include "midi/MidiInput.h"
#include "trainer/RootlessTrainer.h"

namespace {

void logLine(const char* fmt, ...) {
    char buffer[512];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);
    OutputDebugStringA(buffer);
    OutputDebugStringA("\n");
}

// Opens the first enumerated physical (non-virtual) USB-MIDI input device,
// if any is present. Absence of a device is not fatal: the app runs with
// visualization only until one is connected (see reconnect polling below).
bool openFirstPhysicalInput(midi::MidiInput& midiIn, UINT& openedDeviceId, std::string& openedDeviceName) {
    const auto devices = midi::MidiInput::enumeratePhysicalDevices();
    if (devices.empty()) {
        logLine("[MIDI] no physical USB-MIDI input device found");
        return false;
    }
    const auto& chosen = devices.front();
    if (!midiIn.open(chosen.id)) {
        logLine("[MIDI] open failed for device id=%u name=%s", chosen.id, chosen.name.c_str());
        return false;
    }
    openedDeviceId = chosen.id;
    openedDeviceName = chosen.name;
    logLine("[MIDI] opened id=%u name=%s", chosen.id, chosen.name.c_str());
    return true;
}

// docs/UI_REQUIREMENTS.md section 3: five persistent regions, stacked
// top-to-bottom. This is a structural placeholder layout (flat-color
// blocks + real 88-key geometry) proving the DX11 draw path end-to-end;
// actual timeline/waterfall/header content is added by later milestones.
struct RegionLayout {
    float headerY = 0, headerH = 0;
    float timelineY = 0, timelineH = 0;
    float waterfallY = 0, waterfallH = 0;
    float pianoY = 0, pianoH = 0;
    float footerY = 0, footerH = 0;
    float width = 0;
};

RegionLayout computeRegionLayout(float width, float height) {
    RegionLayout layout;
    layout.width = width;

    constexpr float kHeaderH = 48.0f;
    constexpr float kTimelineH = 72.0f;
    constexpr float kFooterH = 28.0f;
    constexpr float kPianoH = 120.0f;
    constexpr float kMinWaterfallH = 60.0f;

    layout.headerY = 0.0f;
    layout.headerH = kHeaderH;

    layout.timelineY = layout.headerY + layout.headerH;
    layout.timelineH = kTimelineH;

    layout.footerH = kFooterH;
    layout.footerY = height - layout.footerH;

    layout.pianoH = kPianoH;
    layout.pianoY = layout.footerY - layout.pianoH;

    layout.waterfallY = layout.timelineY + layout.timelineH;
    layout.waterfallH = layout.pianoY - layout.waterfallY;

    // On a very short/minimized-ish window there is not enough room for
    // every fixed-height region; clamp so nothing draws with negative
    // height rather than producing garbage geometry.
    if (layout.waterfallH < kMinWaterfallH) {
        layout.waterfallH = 0.0f;
        layout.pianoY = layout.timelineY + layout.timelineH;
    }
    return layout;
}

constexpr app::Color4 kColorAppBackground{0.04f, 0.04f, 0.06f, 1.0f};
constexpr app::Color4 kColorHeader{0.13f, 0.15f, 0.20f, 1.0f};
constexpr app::Color4 kColorTimeline{0.10f, 0.12f, 0.17f, 1.0f};
constexpr app::Color4 kColorWaterfall{0.03f, 0.03f, 0.05f, 1.0f};
constexpr app::Color4 kColorPianoPanel{0.09f, 0.09f, 0.12f, 1.0f};
constexpr app::Color4 kColorFooter{0.11f, 0.12f, 0.16f, 1.0f};
constexpr app::Color4 kColorDivider{0.30f, 0.55f, 0.85f, 1.0f};
constexpr app::Color4 kColorWhiteKey{0.90f, 0.90f, 0.88f, 1.0f};
constexpr app::Color4 kColorBlackKey{0.07f, 0.07f, 0.08f, 1.0f};
// Distinguishable from the default key colors and from each other
// (docs/UI_REQUIREMENTS.md sections 16/18: user notes vs. rootless
// violations must both be immediately understandable at a glance).
constexpr app::Color4 kColorKeySounding{0.25f, 0.62f, 0.95f, 1.0f};
constexpr app::Color4 kColorKeyViolation{0.92f, 0.22f, 0.20f, 1.0f};
constexpr app::Color4 kColorMidiConnected{0.25f, 0.80f, 0.35f, 1.0f};
constexpr app::Color4 kColorMidiDisconnected{0.75f, 0.20f, 0.20f, 1.0f};
constexpr float kDividerThickness = 2.0f;

// docs/UI_REQUIREMENTS.md section 4: global header must show, at minimum,
// USB-MIDI connection state. This is a placeholder indicator (no text
// rendering exists yet) but reflects the real MidiInput::state(), not a
// cosmetic stand-in.
app::Color4 midiConnectionIndicatorColor(bool connected) {
    return connected ? kColorMidiConnected : kColorMidiDisconnected;
}

void drawFrame(app::RendererDX11& renderer, float width, float height,
               const std::array<app::NoteVisualState, 128>& noteStates, bool midiConnected) {
    renderer.beginFrame(kColorAppBackground);

    const RegionLayout layout = computeRegionLayout(width, height);

    renderer.drawRect(0, layout.headerY, width, layout.headerH, kColorHeader);

    // MIDI connection indicator, left edge of the header.
    constexpr float kIndicatorSize = 16.0f;
    constexpr float kIndicatorMargin = 16.0f;
    renderer.drawRect(kIndicatorMargin, layout.headerY + (layout.headerH - kIndicatorSize) * 0.5f, kIndicatorSize,
                       kIndicatorSize, midiConnectionIndicatorColor(midiConnected));
    renderer.drawRect(0, layout.timelineY, width, layout.timelineH, kColorTimeline);
    if (layout.waterfallH > 0.0f) {
        renderer.drawRect(0, layout.waterfallY, width, layout.waterfallH, kColorWaterfall);
    }
    renderer.drawRect(0, layout.pianoY, width, layout.pianoH, kColorPianoPanel);
    renderer.drawRect(0, layout.footerY, width, layout.footerH, kColorFooter);

    // Visible separation between major regions (docs/UI_REQUIREMENTS.md
    // section 3/24: hierarchy between regions must remain clear).
    renderer.drawRect(0, layout.timelineY - kDividerThickness, width, kDividerThickness, kColorDivider);
    renderer.drawRect(0, layout.waterfallY - kDividerThickness, width, kDividerThickness, kColorDivider);
    renderer.drawRect(0, layout.pianoY - kDividerThickness, width, kDividerThickness, kColorDivider);
    renderer.drawRect(0, layout.footerY - kDividerThickness, width, kDividerThickness, kColorDivider);

    // 88-key piano (docs/UI_REQUIREMENTS.md section 17), horizontally
    // spanning the full width so waterfall notes above can align by key.
    constexpr float kKeyMarginTop = 8.0f;
    constexpr float kKeyMarginBottom = 8.0f;
    const float keyAreaH = layout.pianoH - kKeyMarginTop - kKeyMarginBottom;
    if (keyAreaH > 0.0f) {
        const float keyTop = layout.pianoY + kKeyMarginTop;
        const auto keys = app::computePianoLayout(width);
        const auto keyColorFor = [&](int midiNote, const app::Color4& idleColor) {
            if (midiNote < 0 || midiNote > 127) {
                return idleColor;
            }
            const auto& state = noteStates[midiNote];
            if (state.violation) {
                return kColorKeyViolation;
            }
            if (state.sounding) {
                return kColorKeySounding;
            }
            return idleColor;
        };

        // White keys first so black keys draw on top, matching real
        // instrument layering.
        for (const auto& key : keys) {
            if (!key.isBlack) {
                renderer.drawRect(key.x, keyTop, key.width - 1.0f, keyAreaH, keyColorFor(key.midiNote, kColorWhiteKey));
            }
        }
        const float blackKeyH = keyAreaH * 0.62f;
        for (const auto& key : keys) {
            if (key.isBlack) {
                renderer.drawRect(key.x, keyTop, key.width, blackKeyH, keyColorFor(key.midiNote, kColorBlackKey));
            }
        }
    }

    renderer.endFrame();
}

}  // namespace

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int) {
    app::Window window;
    if (!window.create(hInstance, L"MIDI Visualizer Bass Engine Studio", 1280, 720)) {
        return 1;
    }

    app::RendererDX11 renderer;
    RECT clientRect{};
    GetClientRect(window.handle(), &clientRect);
    uint32_t clientWidth = static_cast<uint32_t>(clientRect.right - clientRect.left);
    uint32_t clientHeight = static_cast<uint32_t>(clientRect.bottom - clientRect.top);
    if (!renderer.initialize(window.handle(), clientWidth, clientHeight)) {
        window.destroy();
        return 1;
    }
    window.setResizeCallback([&renderer, &clientWidth, &clientHeight](uint32_t width, uint32_t height) {
        renderer.resize(width, height);
        clientWidth = width;
        clientHeight = height;
    });

    midi::MidiInput midiIn;
    midi::MidiEventProcessor eventProcessor;
    UINT midiDeviceId = 0;
    std::string midiDeviceName;
    bool midiConnected = openFirstPhysicalInput(midiIn, midiDeviceId, midiDeviceName);

    // No chord chart is loaded yet (no parser exists), so the trainer runs
    // against an explicit N.C. context: this still exercises the full
    // physical MIDI -> event processor -> application state path without
    // fabricating harmonic data (CLAUDE.md: no speculative frameworks).
    trainer::RootlessTrainer rootlessTrainer;
    rootlessTrainer.applyOct3To5Preset();
    rootlessTrainer.setCurrentChord(timeline::kInvalidPitchClass, /*isNoChord=*/true);

    app::KeyboardState keyboardState;

    DWORD lastReconnectAttemptMs = GetTickCount();
    constexpr DWORD kReconnectIntervalMs = 1000;

    bool running = true;
    while (running) {
        running = window.pumpMessages();
        if (!running) {
            break;
        }

        // Safe disconnect/reconnect handling (docs/USB_MIDI_BRINGUP_TEST.md
        // section 12): never crash on unplug, and allow reopen once the
        // device (or an equivalent physical device) reappears.
        if (midiConnected && midiIn.state() == midi::MidiConnectionState::Disconnected) {
            logLine("[MIDI] device disconnected: %s", midiDeviceName.c_str());
            midiIn.close();
            midiConnected = false;
        }

        if (!midiConnected) {
            const DWORD now = GetTickCount();
            if (now - lastReconnectAttemptMs >= kReconnectIntervalMs) {
                lastReconnectAttemptMs = now;
                midiConnected = openFirstPhysicalInput(midiIn, midiDeviceId, midiDeviceName);
            }
        }

        if (midiConnected) {
            const auto processed = eventProcessor.process(midiIn.drainMessages());
            std::array<bool, 128> wasViolating{};
            for (int note = 0; note < 128; ++note) {
                wasViolating[note] = keyboardState.noteStates()[note].violation;
            }

            keyboardState.apply(processed, rootlessTrainer);

            // Edge-triggered logging: one line per new violation, not one
            // per frame it stays visually highlighted.
            for (int note = 0; note < 128; ++note) {
                if (keyboardState.noteStates()[note].violation && !wasViolating[note]) {
                    logLine("[ROOTLESS] violation note=%d", note);
                }
            }
        }

        drawFrame(renderer, static_cast<float>(clientWidth), static_cast<float>(clientHeight),
                  keyboardState.noteStates(), midiConnected);
    }

    midiIn.close();
    renderer.shutdown();
    window.destroy();
    return 0;
}
