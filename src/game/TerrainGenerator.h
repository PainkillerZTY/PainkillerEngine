#pragma once

#include "Types.h"
#include "Block.h"
#include "Noise.h"
#include "Chunk.h"

namespace nebula {

// ============================================================
// TerrainGenerator: Procedural world generation
// Uses multi-octave noise for height and biome mapping
// ============================================================
class TerrainGenerator {
public:
    TerrainGenerator();
    ~TerrainGenerator() = default;

    void SetSeed(u32 seed);

    // Generate blocks for a chunk (fills the chunk's block data)
    void GenerateChunk(Chunk* chunk, i32 chunkX, i32 chunkZ);

    // Get height at a world position (for placement and player spawn)
    i32 GetHeightAt(i32 worldX, i32 worldZ) const;

    // Get surface block type at a world position
    BlockType GetSurfaceBlock(i32 worldX, i32 worldZ) const;

private:
    // Biome determination (simple temp/moisture-based)
    enum class Biome : u8 {
        Plains,
        Desert,
        Forest,
        Snowy,
        Ocean,
    };

    Biome GetBiome(f32 worldX, f32 worldZ) const;

    // Helper: fill block in chunk
    void SetChunkBlock(Chunk* chunk, i32 localX, i32 localY, i32 localZ, BlockType type);

    Noise m_elevationNoise;
    Noise m_detailNoise;
    Noise m_temperatureNoise;
    Noise m_moistureNoise;
    Noise m_caveNoise;
    u32 m_seed = 42;
};

} // namespace nebula
