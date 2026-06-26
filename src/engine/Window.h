#pragma once

#include "Types.h"
#include <windows.h>
#include <string>
#include <functional>

namespace nebula {

// ?? Window: Win32-based window management ??
class Window {
public:
    Window() = default;
    ~Window() { Shutdown(); }

    bool Initialize(u32 width, u32 height, const std::string& title);
    void Shutdown();
    
    void ProcessMessages();
    bool IsOpen() const { return m_open; }
    void Close() { m_open = false; }
    
    void* GetHandle() const { return m_handle; }
    u32 GetWidth() const { return m_width; }
    u32 GetHeight() const { return m_height; }
    f32 GetAspectRatio() const { return (f32)m_width / (f32)m_height; }
    
    // Callbacks
    using ResizeCallback = std::function<void(u32 width, u32 height)>;
    void SetResizeCallback(ResizeCallback callback) { m_resizeCallback = callback; }

private:
    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    
    HWND m_handle = nullptr;
    HDC  m_hdc = nullptr;
    bool m_open = false;
    u32  m_width = 1280;
    u32  m_height = 720;
    std::string m_title = "Nebula Engine";
    
    ResizeCallback m_resizeCallback;
};

} // namespace nebula



