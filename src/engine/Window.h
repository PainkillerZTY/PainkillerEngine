#pragma once

#include "Types.h"
#include <windows.h>
#include <string>
#include <functional>

namespace painkiller {

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
     
     // Input event callback (raw Win32 messages forwarded to Engine)
     using InputEventCallback = std::function<void(UINT msg, WPARAM wParam, LPARAM lParam)>;
     void SetInputEventCallback(InputEventCallback callback) { m_inputCallback = callback; }

private:
    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    
    HWND m_handle = nullptr;
    HDC  m_hdc = nullptr;
    bool m_open = false;
    u32  m_width = 1280;
    u32  m_height = 720;
     std::string m_title = "Painkiller Engine";
    
     ResizeCallback m_resizeCallback;
     InputEventCallback m_inputCallback;
 };
 
 } // namespace painkiller
 
 // MinGW 6.3 compatibility: Raw Input types (not in old SDK headers)
 #ifndef WM_INPUT
 #define WM_INPUT 0x00FF
 #endif
 #ifndef RIM_TYPEMOUSE
 #define RIM_TYPEMOUSE 0
 #endif
 #ifndef RID_INPUT
 #define RID_INPUT 0x10000003
 #endif
 
 struct MinGW_RAWINPUTHEADER { DWORD dwType; DWORD dwSize; HANDLE hDevice; WPARAM wParam; };
 // WARNING: usButtonFlags/usButtonData share the same memory as ulButtons (union in real WinAPI)
 // We omit them to keep lLastX/lLastY at correct offsets: usFlags(2)+pad(2)+ulButtons(4)+ulRawButtons(4)=12
 struct MinGW_RAWMOUSE { USHORT usFlags; ULONG ulButtons; ULONG ulRawButtons; LONG lLastX; LONG lLastY; ULONG ulExtraInformation; };
 struct MinGW_RAWINPUT { MinGW_RAWINPUTHEADER header; MinGW_RAWMOUSE mouse; };
 struct MinGW_RAWINPUTDEVICE { USHORT usUsagePage; USHORT usUsage; DWORD dwFlags; HWND hwndTarget; };
 
 #ifndef HRAWINPUT
 #define HRAWINPUT HANDLE
 #endif
 
 // Raw Input function pointers (loaded from user32.dll for MinGW 6.3 compat)
 typedef BOOL (WINAPI *RegRawInputDevicesFn)(void*, UINT, UINT);
 typedef UINT (WINAPI *GetRawInputDataFn)(HANDLE, UINT, void*, UINT*, UINT);
 extern RegRawInputDevicesFn pfnRegisterRawInputDevices;
 extern GetRawInputDataFn pfnGetRawInputData;



