// ============================================================
// VoxelGame.h — Main game orchestrator
//
// Top-level game class registered as Engine callbacks.
// Coordinates: World (chunks/terrain), PlayerController,
//   ParticleSystem, SoundManager, all rendering pipelines.
//
// Lifecycle: Initialize() -> Update()/Render() loop -> Shutdown()
// ============================================================
#pragma once

#include "Engine.h"
#include "Camera.h"
#include "World.h"
#include "PlayerController.h"
#include "ParticleSystem.h"
#include "SoundManager.h"
#include "BlockRaycast.h"
#include "Block.h"

namespace painkiller {

// ============================================================
// VoxelGame: Main game orchestrator
// ============================================================
class VoxelGame {
public:
    VoxelGame();
    ~VoxelGame();

    bool Initialize(Engine* engine);
    void Update(Engine* engine, f32 deltaTime);
    void Render(Engine* engine, f32 deltaTime);
    void Shutdown(Engine* engine);

private:
    void HandleBlockInteraction(Engine* engine, f32 deltaTime);
    void UpdateChunks(Engine* engine);
    void SetupShadersAndPipeline(Renderer* renderer);
    void RenderWorld(Renderer* renderer, Camera* camera);
    void RenderUI(Renderer* renderer, f32 winW, f32 winH);
    void RenderCrosshair(Renderer* renderer);
    void RenderHeldBlock(Renderer* renderer, Camera* camera);
    void RenderBlockHighlight(Renderer* renderer, Camera* camera);
    void RenderDebugInfo(Engine* engine);

    // World
    World* m_world = nullptr;

    // Player
    PlayerController m_player;

    // Systems
    ParticleSystem m_particleSystem;
    SoundManager m_soundManager;

    // Shader/pipeline for world rendering
    ShaderHandle m_worldVS = kInvalidHandle;
    ShaderHandle m_worldFS = kInvalidHandle;
    ShaderHandle m_uiVS = kInvalidHandle;
    ShaderHandle m_uiFS = kInvalidHandle;
    PipelineHandle m_worldPipeline = kInvalidHandle;
    PipelineHandle m_uiPipeline = kInvalidHandle;
    ShaderDesc m_worldVSDesc;
    ShaderDesc m_worldFSDesc;

    // UI meshes
    MeshHandle m_crosshairMesh = kInvalidHandle;
    MeshHandle m_whiteQuadMesh = kInvalidHandle;
    MeshHandle m_heldBlockMesh = kInvalidHandle;
    ShaderHandle m_skyboxVS = kInvalidHandle;
    ShaderHandle m_skyboxFS = kInvalidHandle;
    PipelineHandle m_skyboxPipeline = kInvalidHandle;
    MeshHandle m_skyboxMesh = kInvalidHandle;
    ShaderHandle m_wireframeVS = kInvalidHandle;
    ShaderHandle m_wireframeFS = kInvalidHandle;
    PipelineHandle m_wireframePipeline = kInvalidHandle;
    MeshHandle m_wireframeCubeMesh = kInvalidHandle;

    // State
    bool m_initialized = false;
    bool m_pipelineCreated = false;

     // Block interaction
     BlockRaycastResult m_raycastResult;
     
     // Hotbar: selected block index (index into hotbar array)
     i32 m_selectedSlot = 0;
     static constexpr i32 kHotbarSlots = 9;
     void HandleHotbarSelection(Input* input);
     BlockType GetSelectedBlock() const;
     void RenderHotbar(Renderer* renderer, f32 winW, f32 winH);
 
    // Block breaking
    bool m_isBreaking = false;
    f32 m_breakTimer = 0.0f;
    static constexpr f32 kBreakTime = 0.5f; // Seconds to break a block
    i32 m_breakX = 0, m_breakY = 0, m_breakZ = 0;

    // Block placing cooldown
    f32 m_placeCooldown = 0.0f;

     // Texture atlas for block faces
     TextureHandle m_blockAtlas = kInvalidHandle;
     
     // Game state
     u32 m_seed = 42;
     f32 m_gameTime = 0.0f;
};

} // namespace painkiller
