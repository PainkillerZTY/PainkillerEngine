#pragma once

#include "Types.h"
#include "Vector.h"
#include "Block.h"

namespace painkiller {

class World; // Forward declaration

// ============================================================
// BlockRaycastResult: Result of a raycast against the world
// ============================================================
struct BlockRaycastResult {
    bool hit = false;
    i32 blockX = 0, blockY = 0, blockZ = 0; // Hit block
    i32 placeX = 0, placeY = 0, placeZ = 0; // Adjacent block for placement
    BlockFace hitFace = BlockFace::Top;
    f32 distance = 0.0f;
};

// ============================================================
// BlockRaycast: Ray-walking (DDA) for block picking
// ============================================================
class BlockRaycast {
public:
    BlockRaycast() = default;
    ~BlockRaycast() = default;

    // Cast a ray from origin in direction, return first solid block hit
    static BlockRaycastResult Cast(const Vec3& origin, const Vec3& direction,
                                    f32 maxDistance, World* world);

private:
    // Get block type at world position from world
    static BlockType GetBlockAt(World* world, i32 x, i32 y, i32 z);
};

} // namespace painkiller
