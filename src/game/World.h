#pragma once

#include "Types.h"
#include "Chunk.h"
#include "TerrainGenerator.h"
#include <cmath>
#include "Renderer.h"
#include <unordered_map>
#include <vector>
#include <memory>
#include <tuple>

namespace painkiller {


// ============================================================
// Frustum: 6 planes for view frustum culling
// ============================================================
struct FrustumPlane {
    Vec3 normal;
    f32 distance;
};

struct Frustum {
    FrustumPlane planes[6];
    bool Intersects(const Vec3& aabbMin, const Vec3& aabbMax) const {
        for (int i = 0; i < 6; ++i) {
            const auto& p = planes[i];
            f32 px = p.normal.x > 0 ? aabbMax.x : aabbMin.x;
            f32 py = p.normal.y > 0 ? aabbMax.y : aabbMin.y;
            f32 pz = p.normal.z > 0 ? aabbMax.z : aabbMin.z;
            f32 dist = p.normal.x * px + p.normal.y * py + p.normal.z * pz + p.distance;
            if (dist < 0) return false;
        }
        return true;
    }
    static Frustum FromMatrix(const Mat4& vp) {
        Frustum f;
        f.planes[0] = { Vec3(vp[0][3] + vp[0][0], vp[1][3] + vp[1][0], vp[2][3] + vp[2][0]), vp[3][3] + vp[3][0] };
        f.planes[1] = { Vec3(vp[0][3] - vp[0][0], vp[1][3] - vp[1][0], vp[2][3] - vp[2][0]), vp[3][3] - vp[3][0] };
        f.planes[2] = { Vec3(vp[0][3] + vp[0][1], vp[1][3] + vp[1][1], vp[2][3] + vp[2][1]), vp[3][3] + vp[3][1] };
        f.planes[3] = { Vec3(vp[0][3] - vp[0][1], vp[1][3] - vp[1][1], vp[2][3] - vp[2][1]), vp[3][3] - vp[3][1] };
        f.planes[4] = { Vec3(vp[0][3] + vp[0][2], vp[1][3] + vp[1][2], vp[2][3] + vp[2][2]), vp[3][3] + vp[3][2] };
        f.planes[5] = { Vec3(vp[0][3] - vp[0][2], vp[1][3] - vp[1][2], vp[2][3] - vp[2][2]), vp[3][3] - vp[3][2] };
        for (int i = 0; i < 6; ++i) {
            f32 len = std::sqrt(f.planes[i].normal.x * f.planes[i].normal.x +
                                f.planes[i].normal.y * f.planes[i].normal.y +
                                f.planes[i].normal.z * f.planes[i].normal.z);
            f.planes[i].normal /= len;
            f.planes[i].distance /= len;
        }
        return f;
    }
};

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
    void      SetBlocksBatch(const std::vector<std::tuple<i32,i32,i32,BlockType>>& blocks);

    // Render all loaded chunks
    void Render(Renderer* renderer, const Frustum* frustum = nullptr);
    void UpdateWaterPhysics(f32 deltaTime, i32 centerX, i32 centerZ);

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
    f32 m_waterTimer = 0.0f;
    static constexpr f32 kWaterUpdateInterval = 0.25f;

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

} // namespace painkiller
