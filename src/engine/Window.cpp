 #include "Window.h"
 #include "Logger.h"
 
 // Raw Input function pointers (loaded at runtime for MinGW compat)
 RegRawInputDevicesFn pfnRegisterRawInputDevices = nullptr;
 GetRawInputDataFn pfnGetRawInputData = nullptr;

namespace painkiller {

// ?? Window Class Registration ??
static bool g_windowClassRegistered = false;
 static const char* kWindowClassName = "PainkillerEngineWindow";

bool Window::Initialize(u32 width, u32 height, const std::string& title) {
    m_width = width;
    m_height = height;
    m_title = title;
    
    HINSTANCE hInstance = GetModuleHandle(nullptr);
    
    // Register window class
    if (!g_windowClassRegistered) {
        WNDCLASSEXA wc = {};
        wc.cbSize = sizeof(WNDCLASSEXA);
        wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
        wc.lpfnWndProc = WindowProc;
        wc.hInstance = hInstance;
        wc.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
        wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
        wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
        wc.lpszClassName = kWindowClassName;
        
        if (!RegisterClassExA(&wc)) {
            LOG_ERROR("Failed to register window class");
            return false;
        }
        g_windowClassRegistered = true;
    }
    
    // Convert title to wide string
    // Calculate window size with client area
    RECT rect = { 0, 0, (LONG)width, (LONG)height };
    AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, FALSE);
    
    // Create window
    m_handle = CreateWindowExA(
        0,
        kWindowClassName,
        title.c_str(),
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        rect.right - rect.left,
        rect.bottom - rect.top,
        nullptr,
        nullptr,
        hInstance,
        this  // Pass 'this' as creation parameter
    );
    
    if (!m_handle) {
        LOG_ERROR("Failed to create window");
        return false;
    }
    
    m_hdc = GetDC(m_handle);
    m_open = true;
    
    ShowWindow(m_handle, SW_SHOW);
    SetForegroundWindow(m_handle);
    SetFocus(m_handle);
    
     LOG_INFO("Window created: {}x{} - {}", width, height, title);
     
     // Register raw input devices (mouse) for hardware-level deltas
     MinGW_RAWINPUTDEVICE rid = {};
     rid.usUsagePage = 0x01; // HID usage page: Generic Desktop Controls
     rid.usUsage = 0x02;     // HID usage: Mouse
     rid.dwFlags = 0;        // Receive input even when not in foreground
     rid.hwndTarget = m_handle;
     // Load Raw Input functions from user32.dll
     if (!pfnRegisterRawInputDevices) {
         HMODULE hUser32 = GetModuleHandleA("user32.dll");
         if (hUser32) {
             pfnRegisterRawInputDevices = (RegRawInputDevicesFn)GetProcAddress(hUser32, "RegisterRawInputDevices");
             pfnGetRawInputData = (GetRawInputDataFn)GetProcAddress(hUser32, "GetRawInputData");
         }
     }
     if (pfnRegisterRawInputDevices && !pfnRegisterRawInputDevices(&rid, 1, sizeof(rid))) {
         LOG_WARN("Failed to register raw input device");
     }
     
     return true;
}

void Window::Shutdown() {
    if (m_handle) {
        ReleaseDC(m_handle, m_hdc);
        DestroyWindow(m_handle);
        m_handle = nullptr;
        m_open = false;
        LOG_INFO("Window destroyed");
    }
}

void Window::ProcessMessages() {
    MSG msg = {};
    while (PeekMessageA(&msg, nullptr, 0, 0, PM_REMOVE)) {
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
        
        if (msg.message == WM_QUIT) {
            m_open = false;
        }
    }
}

LRESULT CALLBACK Window::WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    Window* window = nullptr;
    
    if (msg == WM_CREATE) {
        CREATESTRUCT* cs = (CREATESTRUCT*)lParam;
        window = (Window*)cs->lpCreateParams;
        SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)window);
    } else {
        window = (Window*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    }
    
     if (!window) return DefWindowProc(hwnd, msg, wParam, lParam);
     
     // Forward input events to the engine via callback
     switch (msg) {
         case WM_KEYDOWN:
         case WM_SYSKEYDOWN:
         case WM_KEYUP:
         case WM_SYSKEYUP:
         case WM_MOUSEMOVE:
         case WM_LBUTTONDOWN:
         case WM_RBUTTONDOWN:
         case WM_MBUTTONDOWN:
         case WM_LBUTTONUP:
         case WM_RBUTTONUP:
         case WM_MBUTTONUP:
     case WM_MOUSEWHEEL:
         case WM_INPUT:
             if (window->m_inputCallback)
                 window->m_inputCallback(msg, wParam, lParam);
             return 0;
     }
     
     switch (msg) {
         case WM_ACTIVATE: {
             // Re-clip cursor when window gains focus for first-person view
             if (LOWORD(wParam) != WA_INACTIVE) {
                 RECT cr;
                 GetClientRect(hwnd, &cr);
                 MapWindowPoints(hwnd, nullptr, (POINT*)&cr, 2);
                 ClipCursor(&cr);
                 ShowCursor(FALSE);
             } else {
                 // Release cursor when window loses focus
                 ClipCursor(nullptr);
                 ShowCursor(TRUE);
             }
             return 0;
         }
         case WM_SIZE: {
             u32 newWidth = LOWORD(lParam);
             u32 newHeight = HIWORD(lParam);
             window->m_width = newWidth;
             window->m_height = newHeight;
             if (window->m_resizeCallback) {
                 window->m_resizeCallback(newWidth, newHeight);
             }
             return 0;
         }
         case WM_CLOSE:
             window->m_open = false;
             return 0;
         case WM_DESTROY:
             PostQuitMessage(0);
             return 0;
        }
     
     return DefWindowProc(hwnd, msg, wParam, lParam);
}

} // namespace painkiller



