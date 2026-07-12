#pragma once

#include "Types.h"
#include "Window.h"
#include "Timer.h"
#include "Input.h"
#include "Renderer.h"
#include "Scene.h"
#include "ResourceManager.h"
#include <memory>
#include <string>

namespace painkiller {

// ?? Engine Configuration ??
struct EngineConfig {
     std::string title = "Painkiller Engine";
    u32 width = 1280;
    u32 height = 720;
    RenderBackend backend = RenderBackend::OpenGL;
    bool vsync = true;
    bool fullscreen = false;
};

// ?? Engine: Core application orchestrator ??
class Engine {
public:
    Engine() = default;
    ~Engine() { Shutdown(); }

    // ?? Lifecycle ??
    bool Initialize(const EngineConfig& config);
    void Shutdown();
    
    // ?? Game Loop ??
    void Run();
    void Stop() { m_running = false; }
    bool IsRunning() const { return m_running; }
    
    // ?? Accessors ??
    Window*          GetWindow()          { return &m_window; }
    Renderer*        GetRenderer()        { return m_renderer.get(); }
    Scene*           GetScene()           { return &m_scene; }
    Timer*           GetTimer()           { return &m_timer; }
    Input*           GetInput()           { return &m_input; }
    ResourceManager* GetResourceManager() { return m_resourceManager.get(); }
    
    // ?? Frame Stats ??
    f32 GetFPS() const { return m_timer.GetFPS(); }
    f32 GetDeltaTime() const { return m_timer.GetDeltaTime(); }
    
    // ?? User Callbacks ??
    using InitCallback    = std::function<bool(Engine*)>;
    using UpdateCallback  = std::function<void(Engine*, f32 deltaTime)>;
    using RenderCallback  = std::function<void(Engine*, f32 deltaTime)>;
    using ShutdownCallback = std::function<void(Engine*)>;
    
    void SetOnInit(InitCallback cb)     { m_onInit = cb; }
    void SetOnUpdate(UpdateCallback cb) { m_onUpdate = cb; }
    void SetOnRender(RenderCallback cb) { m_onRender = cb; }
    void SetOnShutdown(ShutdownCallback cb) { m_onShutdown = cb; }

private:
    void HandleWindowEvents();
    
    Window             m_window;
    Timer              m_timer;
    Input              m_input;
    Scene              m_scene;
    
    std::unique_ptr<Renderer>        m_renderer;
    std::unique_ptr<ResourceManager> m_resourceManager;
    
    EngineConfig m_config;
    bool m_running = false;
    bool m_initialized = false;
    
    // User callbacks
    InitCallback     m_onInit;
    UpdateCallback   m_onUpdate;
    RenderCallback   m_onRender;
    ShutdownCallback m_onShutdown;
};

} // namespace painkiller
