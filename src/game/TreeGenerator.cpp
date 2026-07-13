#include "TreeGenerator.h"
#include "TerrainGenerator.h"
#include "Chunk.h"
#include "Block.h"

namespace painkiller {

void TreeGenerator::SetBlockAt(TerrainGenerator* terrain, Chunk* chunk,
                                i32 chunkX, i32 chunkZ,
                                i32 worldX, i32 worldY, i32 worldZ,
                                BlockType type)
{
    (void)terrain;
    (void)chunkX;
    (void)chunkZ;

    // Calculate local coords within the chunk
    i32 localX = worldX - chunkX * CHUNK_SIZE_X;
    i32 localZ = worldZ - chunkZ * CHUNK_SIZE_Z;

    if (chunk && chunk->IsLocalCoordValid(localX, worldY, localZ)) {
        chunk->SetBlock(localX, worldY, localZ, type);
    }
}

void TreeGenerator::PlaceTree(TerrainGenerator* terrain, Chunk* chunk,
                               i32 chunkX, i32 chunkZ,
                               i32 worldX, i32 worldY, i32 worldZ)
{
    // Oak tree parameters
    i32 trunkHeight = 4 + (worldX & 3); // 4-7 blocks tall
    i32 canopyRadius = 2;

    // Trunk
    for (i32 dy = 1; dy <= trunkHeight; ++dy) {
        SetBlockAt(terrain, chunk, chunkX, chunkZ,
                   worldX, worldY + dy, worldZ,
                   BlockType::OakLog);
    }

    // Canopy (leaves) - layered circles
    i32 topY = worldY + trunkHeight;

    // Top layer: 3x3
    for (i32 dx = -1; dx <= 1; ++dx) {
        for (i32 dz = -1; dz <= 1; ++dz) {
            if (dx == 0 && dz == 0) continue; // Don't place on top of trunk
            SetBlockAt(terrain, chunk, chunkX, chunkZ,
                       worldX + dx, topY, worldZ + dz,
                       BlockType::Leaves);
        }
    }

    // Middle layers: 5x5 (radius 2)
    for (i32 layer = 0; layer < 2; ++layer) {
        i32 ly = topY - 1 - layer;
        for (i32 dx = -canopyRadius; dx <= canopyRadius; ++dx) {
            for (i32 dz = -canopyRadius; dz <= canopyRadius; ++dz) {
                // Skip corners for a rounded look
                i32 dist = abs(dx) + abs(dz);
                if (dist > canopyRadius + 1) continue;
                if (dist > canopyRadius && (rand() & 1)) continue;

                // Don't replace trunk blocks
                BlockType existing = chunk->GetBlock(
                    worldX + dx - chunkX * CHUNK_SIZE_X,
                    ly,
                    worldZ + dz - chunkZ * CHUNK_SIZE_Z);
                if (existing == BlockType::OakLog) continue;

                SetBlockAt(terrain, chunk, chunkX, chunkZ,
                           worldX + dx, ly, worldZ + dz,
                           BlockType::Leaves);
            }
        }
    }
}

} // namespace painkiller
