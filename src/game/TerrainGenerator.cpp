#include "TerrainGenerator.h"
#include <tuple>
#include "TreeGenerator.h"
#include <cmath>
#include <algorithm>

namespace painkiller {

TerrainGenerator::TerrainGenerator()
    : m_elevationNoise(42)
    , m_detailNoise(123)
    , m_temperatureNoise(200)
    , m_moistureNoise(300)
    , m_caveNoise(500)
{
}

void TerrainGenerator::SetSeed(u32 seed) {
    m_seed = seed;
    m_elevationNoise = Noise((i32)seed);
    m_detailNoise = Noise((i32)seed + 81);
    m_temperatureNoise = Noise((i32)seed + 123);
    m_moistureNoise = Noise((i32)seed + 456);
    m_caveNoise = Noise((i32)seed + 789);
}

i32 TerrainGenerator::GetHeightAt(i32 worldX, i32 worldZ) const {
    // Scale coordinates for noise (slightly higher frequency = smoother terrain)
    // Lower frequency = wider hills = smoother terrain
    f32 nx = (f32)worldX * 0.015f;
    f32 nz = (f32)worldZ * 0.015f;

    // Base elevation: lower freq, more octaves for natural smooth rolling hills
    f32 elevation = m_elevationNoise.FBM2D(nx, nz, 4, 2.0f, 0.45f);
    // Gentle micro-variation for subtle surface detail
    f32 detail = m_detailNoise.FBM2D((f32)worldX * 0.04f, (f32)worldZ * 0.04f, 2, 2.0f, 0.4f);

    // Biome-specific height offset (gentler transitions)
    Biome biome = GetBiome((f32)worldX, (f32)worldZ);
    f32 biomeOffset = 0.0f;
    if (biome == Biome::Ocean) biomeOffset = -6.0f;
    else if (biome == Biome::Plains) biomeOffset = 3.0f;
    else if (biome == Biome::Forest) biomeOffset = 5.0f;
    else if (biome == Biome::Snowy) biomeOffset = 4.0f;
    else if (biome == Biome::Desert) biomeOffset = 1.0f;

    // Reduced height range for much smoother terrain
    f32 baseHeight = 36.0f + elevation * 14.0f + biomeOffset;
    f32 detailHeight = detail * 1.2f;  // subtle 0-1.2 height variation

    i32 height = (i32)(baseHeight + detailHeight);

    // Clamp to world bounds
    height = std::max(1, std::min(CHUNK_SIZE_Y - 1, height));

    return height;
}

BlockType TerrainGenerator::GetSurfaceBlock(i32 worldX, i32 worldZ) const {
    Biome biome = GetBiome((f32)worldX, (f32)worldZ);

    switch (biome) {
        case Biome::Desert:  return BlockType::Sand;
        case Biome::Snowy:   return BlockType::Snow;
        case Biome::Forest:
        case Biome::Plains:  return BlockType::Grass;
        case Biome::Ocean:   return BlockType::Sand;
        default:             return BlockType::Grass;
    }
}

TerrainGenerator::Biome TerrainGenerator::GetBiome(f32 worldX, f32 worldZ) const {
    // Use multiple noise samples around the point to smooth biome transitions
    f32 temp = m_temperatureNoise.FBM2D(worldX * 0.004f, worldZ * 0.004f, 3);
    f32 moisture = m_moistureNoise.FBM2D(worldX * 0.004f, worldZ * 0.004f, 3);

    if (temp < -0.3f) return Biome::Snowy;
    if (temp < 0.1f && moisture > 0.2f) return Biome::Forest;
    if (temp > 0.3f && moisture < -0.2f) return Biome::Desert;
    return Biome::Plains;
}

void TerrainGenerator::SetChunkBlock(Chunk* chunk, i32 localX, i32 localY, i32 localZ, BlockType type) {
    if (chunk && chunk->IsLocalCoordValid(localX, localY, localZ)) {
        chunk->SetBlock(localX, localY, localZ, type);
    }
}

void TerrainGenerator::GenerateChunk(Chunk* chunk, i32 chunkX, i32 chunkZ) {
    if (!chunk) return;

    // World-space origin of this chunk
    i32 baseX = chunkX * CHUNK_SIZE_X;
    i32 baseZ = chunkZ * CHUNK_SIZE_Z;

    // Tracking for tree placement
    std::vector<std::tuple<i32,i32,i32>> treePositions;

    for (i32 z = 0; z < CHUNK_SIZE_Z; ++z) {
        for (i32 x = 0; x < CHUNK_SIZE_X; ++x) {
            i32 worldX = baseX + x;
            i32 worldZ = baseZ + z;

            i32 height = GetHeightAt(worldX, worldZ);
            Biome biome = GetBiome((f32)worldX, (f32)worldZ);

            // Fill column from bottom to top
            for (i32 y = 0; y < CHUNK_SIZE_Y; ++y) {
                BlockType block = BlockType::Air;

                if (y == 0) {
                    block = BlockType::Bedrock;
                } else if (y < height - 3) {
                    // Underground - check for caves
                    f32 caveNoise = m_caveNoise.Noise3D((f32)worldX * 0.05f, (f32)y * 0.05f, (f32)worldZ * 0.05f);
                    if (caveNoise > 0.4f && y > 5) {
                        block = BlockType::Air; // Carve cave
                    } else {
                        block = BlockType::Stone;
                    }
                } else if (y < height - 1) {
                    block = BlockType::Dirt;
                } else if (y < height) {
                    // Surface layer
                    block = GetSurfaceBlock(worldX, worldZ);
                } else if (y == height && biome == Biome::Ocean) {
                    block = BlockType::Water;
                } else {
                    block = BlockType::Air;
                }

                SetChunkBlock(chunk, x, y, z, block);
            }

            // Check for tree placement (only in forest biome, with some randomness)
            if (biome == Biome::Forest || biome == Biome::Plains) {
                f32 treeNoise = m_detailNoise.Noise2D((f32)worldX * 0.3f, (f32)worldZ * 0.3f);
                if (treeNoise > 0.35f && biome == Biome::Forest) {
                    treePositions.push_back({worldX, height, worldZ}); // Store height!
                } else if (treeNoise > 0.55f && biome == Biome::Plains) {
                    treePositions.push_back({worldX, height, worldZ}); // Store height!
                }
            }
        }
    }

    // Place trees (using stored height to prevent floating trees)
    TreeGenerator treeGen;
    for (auto& pos : treePositions) {
        i32 wx = std::get<0>(pos);
        i32 wy = std::get<1>(pos);
        i32 wz = std::get<2>(pos);
        f32 r = m_detailNoise.Noise2D((f32)wx * 0.7f, (f32)wz * 0.7f);
        if (r > 0.0f) {
            treeGen.PlaceTree(this, chunk, chunkX, chunkZ, wx, wy, wz);
        }
    }

    chunk->MarkMeshDirty();
}

} // namespace painkiller
