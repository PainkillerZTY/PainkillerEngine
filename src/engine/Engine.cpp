 #include "Engine.h"
 #include "Logger.h"
 #include "OpenGLRenderer.h"
 #include <windowsx.h>

namespace painkiller {

// ?? Initialize ??
 bool Engine::Initialize(const EngineConfig& config) {
     LOG_INFO("Painkiller Engine v{} initializing...", "0.1.0");
    m_config = config;
    
    // 1. Create window
    if (!m_window.Initialize(config.width, config.height, config.title)) {
        LOG_ERROR("Failed to create window");
        return false;
    }
    
    // 2. Create renderer
    switch (config.backend) {
        case RenderBackend::OpenGL:
            m_renderer.reset(new OpenGLRenderer());
            break;
        default:
            LOG_ERROR("Unsupported render backend");
            return false;
    }
    
    // 3. Initialize renderer with window handle
    void* windowHandle = m_window.GetHandle();
    if (!m_renderer->Initialize(config.width, config.height, windowHandle)) {
        LOG_ERROR("Failed to initialize renderer");
        return false;
    }
    
    // 4. Set up window resize callback
     m_window.SetResizeCallback([this](u32 width, u32 height) {
         if (m_renderer) {
             m_renderer->OnResize(width, height);
         }
     });
     
     // Set up input event forwarding from Window → Input
     m_window.SetInputEventCallback([this](UINT msg, WPARAM wParam, LPARAM lParam) {
         switch (msg) {
             case WM_KEYDOWN:
             case WM_SYSKEYDOWN: {
                 u32 key = (u32)wParam;
                 // Windows sends left/right-specific codes; map to generic for simplicity
                 if (key == VK_LSHIFT || key == VK_RSHIFT) key = VK_SHIFT;
                 if (key == VK_LCONTROL || key == VK_RCONTROL) key = VK_CONTROL;
                 m_input.OnKeyDown(key);
                 break;
             }
             case WM_KEYUP:
             case WM_SYSKEYUP: {
                 u32 key = (u32)wParam;
                 if (key == VK_LSHIFT || key == VK_RSHIFT) key = VK_SHIFT;
                 if (key == VK_LCONTROL || key == VK_RCONTROL) key = VK_CONTROL;
                 m_input.OnKeyUp(key);
                 break;
             }
             case WM_MOUSEMOVE:
                 m_input.OnMouseMove(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
                 break;
             case WM_LBUTTONDOWN:
                 m_input.OnMouseDown(0);
                 break;
             case WM_LBUTTONUP:
                 m_input.OnMouseUp(0);
                 break;
             case WM_RBUTTONDOWN:
                 m_input.OnMouseDown(1);
                 break;
             case WM_RBUTTONUP:
                 m_input.OnMouseUp(1);
                 break;
             case WM_MBUTTONDOWN:
                 m_input.OnMouseDown(2);
                 break;
             case WM_MBUTTONUP:
                 m_input.OnMouseUp(2);
                 break;
             case WM_MOUSEWHEEL:
                 m_input.OnScroll((i32)(GET_WHEEL_DELTA_WPARAM(wParam) / WHEEL_DELTA));
                 break;
             case WM_INPUT: {
                 MinGW_RAWINPUT raw;
                 UINT size = sizeof(raw);
                 if (pfnGetRawInputData) {
                     pfnGetRawInputData((HANDLE)lParam, RID_INPUT, &raw, &size, sizeof(MinGW_RAWINPUTHEADER));
                     if (raw.header.dwType == RIM_TYPEMOUSE) {
                         m_input.OnRawMouseDelta(raw.mouse.lLastX, raw.mouse.lLastY);
                     }
                 }
                 break;
             }
         }
     });

    // 5. Create resource manager
    m_resourceManager = std::make_unique<ResourceManager>(m_renderer.get());
    
    // 6. Configure scene camera
    m_scene.GetActiveCamera()->SetPerspective(60.0f, m_window.GetAspectRatio(), 0.1f, 100.0f);
    m_scene.GetActiveCamera()->SetPosition(Vec3(0.0f, 2.0f, 5.0f));
    
    // 7. Call user init
    m_initialized = true;
    if (m_onInit && !m_onInit(this)) {
        LOG_ERROR("User initialization failed");
        return false;
    }
    
     LOG_INFO("Painkiller Engine initialized successfully");
    return true;
}

void Engine::Shutdown() {
    if (!m_initialized) return;
    
    if (m_onShutdown) m_onShutdown(this);
    
    m_resourceManager.reset();
    m_renderer.reset();
    m_window.Shutdown();
    
     m_initialized = false;
     LOG_INFO("Painkiller Engine shut down");
}

// ?? Handle Windows Messages ??
void Engine::HandleWindowEvents() {
     m_window.ProcessMessages();
}

// ?? Main Loop ??
void Engine::Run() {
    if (!m_initialized) {
        LOG_ERROR("Engine not initialized");
        return;
    }
    
    m_running = true;
    m_timer.Reset();
    
     LOG_INFO("Engine entering main loop");
     
     // Hide cursor for first-person mouse look
     ShowCursor(FALSE);
    
    while (m_running && m_window.IsOpen()) {
        // ?? Begin Frame ??
        m_timer.Tick();
        m_input.BeginFrame();
        
        // ?? Process Window Events ??
        m_window.ProcessMessages();
        if (!m_window.IsOpen()) {
         m_running = false;
         break;
        }
        
         // No cursor warp needed — Raw Input gives us hardware-level deltas
        
        f32 dt = m_timer.GetDeltaTime();

        // ?? Update ??
        if (m_onUpdate) m_onUpdate(this, dt);
        
        // ?? Render ??
        m_renderer->BeginFrame();
         m_renderer->ClearColor(0.5f, 0.7f, 0.95f, 1.0f);
        m_renderer->ClearDepth(1.0f);
        
        if (m_onRender) m_onRender(this, dt);
        
        m_scene.RenderScene(m_renderer.get(), dt);
        
        m_renderer->EndFrame();
        m_renderer->Present();
        
        // ?? Frame Limit / Sleep ??
        // (Optional: frame limiter can be added here)
    }
    
     ClipCursor(nullptr);
     ShowCursor(TRUE);
     LOG_INFO("Engine main loop ended");
}

} // namespace painkiller
