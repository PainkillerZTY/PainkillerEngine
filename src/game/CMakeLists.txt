# Game Module: Voxel Game (Minecraft-like)
add_executable(painkiller
    main.cpp
    Block.h Block.cpp
    Noise.h Noise.cpp
    Chunk.h Chunk.cpp
    World.h World.cpp
    TerrainGenerator.h TerrainGenerator.cpp
    TreeGenerator.h TreeGenerator.cpp
    PlayerController.h PlayerController.cpp
    BlockRaycast.h BlockRaycast.cpp
    Packet.h Packet.cpp
    Network.h Network.cpp
    ParticleSystem.h ParticleSystem.cpp
    SoundManager.h SoundManager.cpp
    VoxelGame.h VoxelGame.cpp
)

target_include_directories(painkiller PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(painkiller PRIVATE painkiller_engine ws2_32)
