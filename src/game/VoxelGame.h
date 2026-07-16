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
#include "Network.h"
#include "BlockRaycast.h"
#include "Block.h"

namespace painkiller {

// Mob type for hostile/passive entities
enum class MobType : u8 { Passive = 0, Zombie, Skeleton, Creeper };

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
    void RenderPlayerModel(Renderer* renderer, Camera* camera, f32 deltaTime);
    void RenderDebugInfo(Engine* engine, f32 deltaTime);
    void RenderDebugScreen(Renderer* renderer, f32 winW, f32 winH);
    void RenderDrops(Renderer* renderer, Camera* camera);
    void RenderMobs(Renderer* renderer, Camera* camera, f32 deltaTime);
    void UpdateMobs(f32 deltaTime);
    void UpdateDrops(f32 deltaTime);
    void SpawnBlockDrop(const Vec3& pos, BlockType type);
    f32 GetBreakMultiplier() const;
    void UpdateSurvival(f32 deltaTime);
    void ApplyFallDamage();
    void RenderHealthBar(Renderer* renderer, f32 winW, f32 winH);
    void RenderInventoryScreen(Renderer* renderer, f32 winW, f32 winH);
    void RenderCraftingScreen(Renderer* renderer, f32 winW, f32 winH);
    void RenderFurnaceScreen(Renderer* renderer, f32 winW, f32 winH);
    void SpawnFoodDrop(const Vec3& pos);
    void SpawnHostileMob(const Vec3& pos, MobType type);
    void CollectItem(BlockType type);
    bool HasItem(BlockType type) const;

    // World
    World* m_world = nullptr;

    // Player
    PlayerController m_player;

    // Systems
    ParticleSystem m_particleSystem;
    SoundManager m_soundManager;
    NetworkServer m_networkServer;

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
    ShaderHandle m_playerVS = kInvalidHandle;
    ShaderHandle m_playerFS = kInvalidHandle;
    PipelineHandle m_playerPipeline = kInvalidHandle;
    MeshHandle m_playerCubeMesh = kInvalidHandle;

    // State
    bool m_initialized = false;
    bool m_pipelineCreated = false;

     // Block interaction
     BlockRaycastResult m_raycastResult;
     
     // Hotbar: selected block index (index into hotbar array)
     i32 m_selectedSlot = 0;
     BlockType m_hotbarBlocks[9];
     static constexpr i32 kHotbarSlots = 9;
     void HandleHotbarSelection(Input* input);
     BlockType GetSelectedBlock() const;
     void RenderHotbar(Renderer* renderer, f32 winW, f32 winH);
 
    // Block breaking
    bool m_isBreaking = false;
    f32 m_breakTimer = 0.0f;
    static constexpr f32 kBreakTime = 0.5f; // Seconds to break a block
    i32 m_breakX = 0, m_breakY = 0, m_breakZ = 0;

    // Debug mode (F3)
    bool m_isDebugMode = false;
    
    // Health & Survival
    f32 m_health = 20.0f;
    f32 m_maxHealth = 20.0f;
    f32 m_hunger = 20.0f;
    f32 m_prevY = 0.0f;
    bool m_wasOnGround = true;
    bool m_inventoryOpen = false;
    bool m_craftingOpen = false;
    bool m_furnaceOpen = false;
    f32 m_smeltTimer = 0.0f;
    
    // Inventory (BlockType -> count)
    static constexpr i32 kMaxInventory = 128;
    i32 m_inventory[128] = {};  // indexed by BlockType enum value
    f32 m_lastDamageTime = 0.0f;
    
    // Mob & Drop system
    struct DropItem { Vec3 pos; Vec3 vel; f32 life; BlockType blockType; bool collected; };
    struct MobEntity { Vec3 pos; Vec3 vel; float scale; float r,g,b; float phase; MobType mobType; float health; float attackTimer; };
    std::vector<MobEntity> m_mobEntities;
    std::vector<DropItem> m_dropItems;
    MeshHandle m_dropItemMesh = kInvalidHandle;
    f32 m_mobSpawnTimer = 0.0f;
    
    // Block placing cooldown
    f32 m_placeCooldown = 0.0f;

     // Texture atlas for block faces
     // Mob rendering
    ShaderHandle m_mobVS = kInvalidHandle;
    ShaderHandle m_mobFS = kInvalidHandle;
    PipelineHandle m_mobPipeline = kInvalidHandle;
    MeshHandle m_mobMesh = kInvalidHandle;
    // Mob type must be declared before methods that use it
    
    // Texture handle
    TextureHandle m_blockAtlas = kInvalidHandle;
     
     // Game state
     u32 m_seed = 42;
     f32 m_gameTime = 0.0f;
};

} // namespace painkiller
