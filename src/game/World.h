#pragma once

#include "Types.h"
#include "Chunk.h"
#include "TerrainGenerator.h"
#include "Renderer.h"
#include <unordered_map>
#include <vector>
#include <memory>

namespace nebula {

// ============================================================
// World: Manages chunks, terrain generation, block I/O
// ============================================================
class World {
public:
    World(Renderer* renderer);
    ~World();

    // Initialize with a seed
    void Initialize(u32 seed);

    // Chunk management
    Chunk* GetChunk(i32 chunkX, i32 chunkZ);
    Chunk* LoadChunk(i32 chunkX, i32 chunkZ);
    void UnloadChunk(i32 chunkX, i32 chunkZ);
    bool IsChunkLoaded(i32 chunkX, i32 chunkZ) const;

    // Ensure chunks around a position are loaded
    void UpdateChunksAround(i32 centerBlockX, i32 centerBlockZ, i32 radius = 6);

    // Block get/set across chunk boundaries
    BlockType GetBlock(i32 worldX, i32 worldY, i32 worldZ) const;
    void      SetBlock(i32 worldX, i32 worldY, i32 worldZ, BlockType type);

    // Render all loaded chunks
    void Render(Renderer* renderer);

    // Renderer pointer
    Renderer* GetRenderer() const { return m_renderer; }

    // Seed
    u32 GetSeed() const { return m_seed; }

    // Access terrain generator
    TerrainGenerator& GetTerrainGenerator() { return m_terrainGen; }

    // Get all loaded chunks
    std::vector<Chunk*> GetAllChunks();

private:
    // Build mesh for a chunk and upload to GPU
    void UploadChunkMesh(Chunk* chunk);

    // Internal chunk storage
    std::unordered_map<ChunkPos, std::unique_ptr<Chunk>, ChunkPos::Hash> m_chunks;

    // Render resources per chunk (mesh handles)
    struct ChunkRenderData {
        MeshHandle meshHandle = kInvalidHandle;
    };
    std::unordered_map<ChunkPos, ChunkRenderData, ChunkPos::Hash> m_chunkRenderData;

    Renderer* m_renderer = nullptr;
    u32 m_seed = 42;
    TerrainGenerator m_terrainGen;
    bool m_initialized = false;
};

} // namespace nebula
