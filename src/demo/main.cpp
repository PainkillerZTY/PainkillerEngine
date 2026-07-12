#include "Engine.h"
#include "DemoApp.h"
#include "Logger.h"
#include <cstdlib>

int main() {
    // Configure engine
    painkiller::EngineConfig config;
    config.title = "Nebula Engine v0.1.0 - Demo";
    config.width = 1280;
    config.height = 720;
    config.backend = painkiller::RenderBackend::OpenGL;
    config.vsync = true;

    // Create engine and demo
    painkiller::Engine engine;
    painkiller::DemoApp demo;

    // Wire up callbacks
    engine.SetOnInit([&demo](painkiller::Engine* engine) -> bool {
        painkiller::Logger::Instance().SetLevel(painkiller::LogLevel::Debug);
        return demo.Initialize(engine);
    });

    engine.SetOnUpdate([&demo](painkiller::Engine* engine, painkiller::f32 dt) {
        demo.Update(engine, dt);
    });

    engine.SetOnRender([&demo](painkiller::Engine* engine, painkiller::f32 dt) {
        demo.Render(engine, dt);
    });

    engine.SetOnShutdown([&demo](painkiller::Engine* engine) {
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

