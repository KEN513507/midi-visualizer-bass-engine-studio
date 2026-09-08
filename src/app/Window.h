#pragma once

#include <functional>
#include <windows.h>

namespace app {

// Win32 window lifecycle only. No rendering, no input processing beyond
// forwarding raw messages to callbacks.
class Window {
public:
    using ResizeCallback = std::function<void(uint32_t width, uint32_t height)>;

    Window() = default;
    ~Window();

    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;

    bool create(HINSTANCE instance, const wchar_t* title, int width, int height);
    void destroy();

    HWND handle() const { return hwnd_; }

    void setResizeCallback(ResizeCallback callback) { onResize_ = std::move(callback); }

    // Pumps all pending messages. Returns false when a WM_QUIT was received.
    bool pumpMessages();

private:
    static LRESULT CALLBACK staticWndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);
    LRESULT handleMessage(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

    HWND hwnd_ = nullptr;
    ResizeCallback onResize_;
    bool quitReceived_ = false;
    static constexpr const wchar_t* kWindowClassName = L"MidiVisualizerBassEngineStudioWindowClass";
};

}  // namespace app
