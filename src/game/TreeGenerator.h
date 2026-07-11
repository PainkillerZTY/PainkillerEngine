#pragma once

#include "Types.h"
#include "Block.h"
#include "Chunk.h"

namespace nebula {

class TerrainGenerator; // Forward declaration

// ============================================================
// TreeGenerator: Places trees on the terrain
// ============================================================
class TreeGenerator {
public:
    TreeGenerator() = default;
    ~TreeGenerator() = default;

    // Place an oak tree at the given world position
    void PlaceTree(TerrainGenerator* terrain, Chunk* chunk,
                   i32 chunkX, i32 chunkZ,
                   i32 worldX, i32 worldY, i32 worldZ);

private:
    void SetBlockAt(TerrainGenerator* terrain, Chunk* chunk,
                    i32 chunkX, i32 chunkZ,
                    i32 worldX, i32 worldY, i32 worldZ,
                    BlockType type);
};

} // namespace nebula
