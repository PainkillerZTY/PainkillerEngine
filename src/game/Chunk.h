#pragma once

#include "Types.h"
#include "Block.h"
#include <vector>
#include <array>

namespace painkiller {

// ============================================================
// Chunk constants
// ============================================================
constexpr i32 CHUNK_SIZE_X = 16;
constexpr i32 CHUNK_SIZE_Y = 128;
constexpr i32 CHUNK_SIZE_Z = 16;
constexpr i32 CHUNK_VOLUME = CHUNK_SIZE_X * CHUNK_SIZE_Y * CHUNK_SIZE_Z;

// ============================================================
// Chunk Position (in chunk coordinates)
// ============================================================
struct ChunkPos {
    i32 x, z;

    ChunkPos() : x(0), z(0) {}
    ChunkPos(i32 x, i32 z) : x(x), z(z) {}

    bool operator==(const ChunkPos& o) const { return x == o.x && z == o.z; }
    bool operator!=(const ChunkPos& o) const { return !(*this == o); }

    // Hash for unordered_map
    struct Hash {
        usize operator()(const ChunkPos& p) const {
            return ((usize)(p.x) << 16) ^ (usize)(p.z);
        }
    };
};

// ============================================================
// Chunk: 16x128x16 block storage + mesh generation
// ============================================================
class Chunk {
public:
    Chunk() = default;
    Chunk(i32 chunkX, i32 chunkZ);
    ~Chunk() = default;

    // Block access
    BlockType GetBlock(i32 x, i32 y, i32 z) const;
    void      SetBlock(i32 x, i32 y, i32 z, BlockType type);
    bool      IsLocalCoordValid(i32 x, i32 y, i32 z) const;

    // Mesh generation
    void GenerateMesh(i32 chunkX, i32 chunkZ);
    bool IsMeshDirty() const { return m_meshDirty; }
    void ClearMeshDirty() { m_meshDirty = false; }
    void MarkMeshDirty() { m_meshDirty = true; }

    // Access generated mesh data
    const std::vector<f32>& GetVertexData() const { return m_vertices; }
    const std::vector<u32>& GetIndexData() const { return m_indices; }
    u32 GetVertexCount() const { return m_vertexCount; }
    u32 GetIndexCount() const { return m_indexCount; }
    u32 GetVertexStride() const { return 8 * sizeof(f32); } // pos(3) + normal(3) + texcoord(2)

    // Position
    i32 GetChunkX() const { return m_chunkX; }
    i32 GetChunkZ() const { return m_chunkZ; }
    void SetPosition(i32 cx, i32 cz) { m_chunkX = cx; m_chunkZ = cz; }

    // Has geometry?
    bool HasMesh() const { return m_hasMesh; }

private:
    // Convert local coords to flat index
    static i32 BlockIndex(i32 x, i32 y, i32 z) {
        return (y * CHUNK_SIZE_Z + z) * CHUNK_SIZE_X + x;
    }

    // Add a face quad to the mesh data
    void AddFace(const Vec3& v0, const Vec3& v1, const Vec3& v2, const Vec3& v3,
                 const Vec3& normal,
                 BlockType blockType, BlockFace face);

    i32 m_chunkX = 0;
    i32 m_chunkZ = 0;

    std::array<BlockType, CHUNK_VOLUME> m_blocks = {};

    // Generated mesh data
    std::vector<f32> m_vertices;
    std::vector<u32> m_indices;
    u32 m_vertexCount = 0;
    u32 m_indexCount = 0;
    bool m_hasMesh = false;

    bool m_meshDirty = true;
};

// ============================================================
// Helper: Convert between world and chunk/block coordinates
// ============================================================
inline void WorldToChunkCoords(i32 worldX, i32 worldY, i32 worldZ,
                                i32& chunkX, i32& chunkZ,
                                i32& localX, i32& localY, i32& localZ) {
    chunkX = (worldX >= 0) ? worldX / CHUNK_SIZE_X : (worldX + 1) / CHUNK_SIZE_X - 1;
    chunkZ = (worldZ >= 0) ? worldZ / CHUNK_SIZE_Z : (worldZ + 1) / CHUNK_SIZE_Z - 1;
    localX = worldX - chunkX * CHUNK_SIZE_X;
    localY = worldY;
    localZ = worldZ - chunkZ * CHUNK_SIZE_Z;
}

} // namespace painkiller
