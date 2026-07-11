#include "Engine.h"
#include "VoxelGame.h"
#include "Logger.h"
#include <cstdlib>

int main() {
    nebula::EngineConfig config;
    config.title = "Nebula Engine - Minecraft-Like Demo";
    config.width = 1280;
    config.height = 720;
    config.backend = nebula::RenderBackend::OpenGL;
    config.vsync = true;

    nebula::Engine engine;
    nebula::VoxelGame game;

    engine.SetOnInit([&game](nebula::Engine* engine) -> bool {
        nebula::Logger::Instance().SetLevel(nebula::LogLevel::Info);
        return game.Initialize(engine);
    });

    engine.SetOnUpdate([&game](nebula::Engine* engine, nebula::f32 dt) {
        game.Update(engine, dt);
    });

    engine.SetOnRender([&game](nebula::Engine* engine, nebula::f32 dt) {
        game.Render(engine, dt);
    });

    engine.SetOnShutdown([&game](nebula::Engine* engine) {
        game.Shutdown(engine);
    });

    if (!engine.Initialize(config)) {
        LOG_ERROR("Engine initialization failed");
        return EXIT_FAILURE;
    }

    engine.Run();

    return EXIT_SUCCESS;
}
