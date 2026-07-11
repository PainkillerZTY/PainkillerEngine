# ?? Game Module: Voxel Game (Minecraft-like) ??
add_executable(nebula_game
    main.cpp
    Block.h Block.cpp
    Noise.h Noise.cpp
    Chunk.h Chunk.cpp
    World.h World.cpp
    TerrainGenerator.h TerrainGenerator.cpp
    TreeGenerator.h TreeGenerator.cpp
    PlayerController.h PlayerController.cpp
    BlockRaycast.h BlockRaycast.cpp
    ParticleSystem.h ParticleSystem.cpp
    SoundManager.h SoundManager.cpp
    VoxelGame.h VoxelGame.cpp
)

target_include_directories(nebula_game PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(nebula_game PRIVATE nebula_engine)
