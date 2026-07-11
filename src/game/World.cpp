#include "World.h"
#include "Chunk.h"
#include "Logger.h"
#include <algorithm>

namespace nebula {

World::World(Renderer* renderer)
    : m_renderer(renderer)
{
}

World::~World() {
    // Clean up all chunk render data
    for (auto& [pos, data] : m_chunkRenderData) {
        if (data.meshHandle != kInvalidHandle && m_renderer) {
            m_renderer->DestroyMesh(data.meshHandle);
        }
    }
    m_chunkRenderData.clear();
    m_chunks.clear();
}

void World::Initialize(u32 seed) {
    m_seed = seed;
    m_terrainGen.SetSeed(seed);
    m_initialized = true;
    LOG_INFO("World initialized with seed: {}", seed);
}

// ============================================================
// Chunk Management
// ============================================================
Chunk* World::GetChunk(i32 chunkX, i32 chunkZ) {
    auto it = m_chunks.find(ChunkPos(chunkX, chunkZ));
    return it != m_chunks.end() ? it->second.get() : nullptr;
}

bool World::IsChunkLoaded(i32 chunkX, i32 chunkZ) const {
    return m_chunks.find(ChunkPos(chunkX, chunkZ)) != m_chunks.end();
}

Chunk* World::LoadChunk(i32 chunkX, i32 chunkZ) {
    if (IsChunkLoaded(chunkX, chunkZ)) {
        return GetChunk(chunkX, chunkZ);
    }

    auto chunk = std::make_unique<Chunk>(chunkX, chunkZ);

    // Generate terrain
    m_terrainGen.GenerateChunk(chunk.get(), chunkX, chunkZ);

    // Generate mesh data
    chunk->GenerateMesh(chunkX, chunkZ);

    // Store chunk
    ChunkPos pos(chunkX, chunkZ);
    Chunk* ptr = chunk.get();
    m_chunks[pos] = std::move(chunk);

    // Upload mesh to GPU
    UploadChunkMesh(ptr);

    return ptr;
}

void World::UnloadChunk(i32 chunkX, i32 chunkZ) {
    ChunkPos pos(chunkX, chunkZ);

    auto renderIt = m_chunkRenderData.find(pos);
    if (renderIt != m_chunkRenderData.end()) {
        if (renderIt->second.meshHandle != kInvalidHandle && m_renderer) {
            m_renderer->DestroyMesh(renderIt->second.meshHandle);
        }
        m_chunkRenderData.erase(renderIt);
    }

    m_chunks.erase(pos);
}

void World::UpdateChunksAround(i32 centerBlockX, i32 centerBlockZ, i32 radius) {
    i32 centerChunkX = (centerBlockX >= 0) ? centerBlockX / CHUNK_SIZE_X :
                        (centerBlockX + 1) / CHUNK_SIZE_X - 1;
    i32 centerChunkZ = (centerBlockZ >= 0) ? centerBlockZ / CHUNK_SIZE_Z :
                        (centerBlockZ + 1) / CHUNK_SIZE_Z - 1;

    // Load chunks within radius
    for (i32 dx = -radius; dx <= radius; ++dx) {
        for (i32 dz = -radius; dz <= radius; ++dz) {
            i32 cx = centerChunkX + dx;
            i32 cz = centerChunkZ + dz;
            if (!IsChunkLoaded(cx, cz)) {
                LoadChunk(cx, cz);
            }
        }
    }

    // Unload chunks outside radius (with margin)
    // Keep a slightly larger ring for hysteresis
    const i32 unloadRadius = radius + 2;
    std::vector<ChunkPos> toUnload;
    for (auto& [pos, chunk] : m_chunks) {
        i32 dx = pos.x - centerChunkX;
        i32 dz = pos.z - centerChunkZ;
        if (abs(dx) > unloadRadius || abs(dz) > unloadRadius) {
            toUnload.push_back(pos);
        }
    }

    for (auto& pos : toUnload) {
        UnloadChunk(pos.x, pos.z);
    }
}

// ============================================================
// Block I/O (world coordinates)
// ============================================================
BlockType World::GetBlock(i32 worldX, i32 worldY, i32 worldZ) const {
    if (worldY < 0 || worldY >= CHUNK_SIZE_Y) return BlockType::Air;

    i32 chunkX, chunkZ, localX, localY, localZ;
    WorldToChunkCoords(worldX, worldY, worldZ, chunkX, chunkZ, localX, localY, localZ);

    auto it = m_chunks.find(ChunkPos(chunkX, chunkZ));
    if (it != m_chunks.end()) {
        return it->second->GetBlock(localX, localY, localZ);
    }
    return BlockType::Air;
}

void World::SetBlock(i32 worldX, i32 worldY, i32 worldZ, BlockType type) {
    if (worldY < 0 || worldY >= CHUNK_SIZE_Y) return;

    i32 chunkX, chunkZ, localX, localY, localZ;
    WorldToChunkCoords(worldX, worldY, worldZ, chunkX, chunkZ, localX, localY, localZ);

    auto it = m_chunks.find(ChunkPos(chunkX, chunkZ));
    if (it != m_chunks.end()) {
        it->second->SetBlock(localX, localY, localZ, type);
        it->second->GenerateMesh(chunkX, chunkZ);
        UploadChunkMesh(it->second.get());
    }
}

// ============================================================
// GPU Mesh Upload
// ============================================================
void World::UploadChunkMesh(Chunk* chunk) {
    if (!m_renderer || !chunk) return;

    ChunkPos pos(chunk->GetChunkX(), chunk->GetChunkZ());

    // Destroy old mesh
    auto renderIt = m_chunkRenderData.find(pos);
    if (renderIt != m_chunkRenderData.end()) {
        if (renderIt->second.meshHandle != kInvalidHandle) {
            m_renderer->DestroyMesh(renderIt->second.meshHandle);
        }
        m_chunkRenderData.erase(renderIt);
    }

    // Create new mesh if chunk has geometry
    if (!chunk->HasMesh()) return;

    MeshData meshData;
    meshData.vertices = chunk->GetVertexData();
    meshData.indices = chunk->GetIndexData();
    meshData.vertexCount = chunk->GetVertexCount();
    meshData.indexCount = chunk->GetIndexCount();
    meshData.vertexStride = chunk->GetVertexStride();

    MeshHandle handle = m_renderer->CreateMesh(meshData);
    if (handle != kInvalidHandle) {
        ChunkRenderData renderData;
        renderData.meshHandle = handle;
        m_chunkRenderData[pos] = renderData;
    }
}

// ============================================================
// Rendering
// ============================================================
void World::Render(Renderer* renderer) {
    if (!renderer) return;

    for (auto& [pos, data] : m_chunkRenderData) {
        if (data.meshHandle != kInvalidHandle) {
            renderer->DrawMesh(data.meshHandle);
        }
    }
}

std::vector<Chunk*> World::GetAllChunks() {
    std::vector<Chunk*> result;
    for (auto& [pos, chunk] : m_chunks) {
        result.push_back(chunk.get());
    }
    return result;
}

} // namespace nebula
