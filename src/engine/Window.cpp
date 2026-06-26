#include "Window.h"
#include "Logger.h"

namespace nebula {

// ?? Window Class Registration ??
static bool g_windowClassRegistered = false;
static const char* kWindowClassName = "NebulaEngineWindow";

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
    
    if (window) {
        switch (msg) {
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
    }
    
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

} // namespace nebula



