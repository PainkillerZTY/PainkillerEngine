#include "Engine.h"
#include "VoxelGame.h"
#include "Logger.h"
#include <cstdlib>

int main() {
    painkiller::EngineConfig config;
     config.title = "Painkiller Engine - Minecraft-Like Demo";
    config.width = 1280;
    config.height = 720;
    config.backend = painkiller::RenderBackend::OpenGL;
    config.vsync = true;

    painkiller::Engine engine;
    painkiller::VoxelGame game;

    engine.SetOnInit([&game](painkiller::Engine* engine) -> bool {
        painkiller::Logger::Instance().SetLevel(painkiller::LogLevel::Info);
        return game.Initialize(engine);
    });

    engine.SetOnUpdate([&game](painkiller::Engine* engine, painkiller::f32 dt) {
        game.Update(engine, dt);
    });

    engine.SetOnRender([&game](painkiller::Engine* engine, painkiller::f32 dt) {
        game.Render(engine, dt);
    });

    engine.SetOnShutdown([&game](painkiller::Engine* engine) {
        game.Shutdown(engine);
    });

    if (!engine.Initialize(config)) {
        LOG_ERROR("Engine initialization failed");
        return EXIT_FAILURE;
    }

    engine.Run();

    return EXIT_SUCCESS;
}
