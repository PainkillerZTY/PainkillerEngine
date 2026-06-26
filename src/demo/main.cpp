#include "Engine.h"
#include "DemoApp.h"
#include "Logger.h"
#include <cstdlib>

int main() {
    // Configure engine
    nebula::EngineConfig config;
    config.title = "Nebula Engine v0.1.0 - Demo";
    config.width = 1280;
    config.height = 720;
    config.backend = nebula::RenderBackend::OpenGL;
    config.vsync = true;

    // Create engine and demo
    nebula::Engine engine;
    nebula::DemoApp demo;

    // Wire up callbacks
    engine.SetOnInit([&demo](nebula::Engine* engine) -> bool {
        nebula::Logger::Instance().SetLevel(nebula::LogLevel::Debug);
        return demo.Initialize(engine);
    });

    engine.SetOnUpdate([&demo](nebula::Engine* engine, nebula::f32 dt) {
        demo.Update(engine, dt);
    });

    engine.SetOnRender([&demo](nebula::Engine* engine, nebula::f32 dt) {
        demo.Render(engine, dt);
    });

    engine.SetOnShutdown([&demo](nebula::Engine* engine) {
        demo.Shutdown(engine);
    });

    // Run
    if (!engine.Initialize(config)) {
        LOG_ERROR("Engine initialization failed");
        return EXIT_FAILURE;
    }

    engine.Run();

    return EXIT_SUCCESS;
}

