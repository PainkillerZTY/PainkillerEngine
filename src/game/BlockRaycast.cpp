#include "BlockRaycast.h"
#include "World.h"
#include "Chunk.h"
#include <cmath>

namespace painkiller {

BlockType BlockRaycast::GetBlockAt(World* world, i32 x, i32 y, i32 z) {
    return world ? world->GetBlock(x, y, z) : BlockType::Air;
}

BlockRaycastResult BlockRaycast::Cast(const Vec3& origin, const Vec3& direction,
                                       f32 maxDistance, World* world)
{
    BlockRaycastResult result;

    if (!world) return result;

    // DDA (Digital Differential Analyzer) algorithm for voxel traversal
    f32 ox = origin.x, oy = origin.y, oz = origin.z;
    f32 dx = direction.x, dy = direction.y, dz = direction.z;

    // Current block position
    i32 bx = (i32)floorf(ox);
    i32 by = (i32)floorf(oy);
    i32 bz = (i32)floorf(oz);

    // Step direction
    i32 stepX = (dx > 0) ? 1 : ((dx < 0) ? -1 : 0);
    i32 stepY = (dy > 0) ? 1 : ((dy < 0) ? -1 : 0);
    i32 stepZ = (dz > 0) ? 1 : ((dz < 0) ? -1 : 0);

    // Distance along ray to next voxel boundary
    f32 tMaxX = (dx != 0) ? (((bx + (stepX > 0 ? 1 : 0)) - ox) / dx) : INFINITY;
    f32 tMaxY = (dy != 0) ? (((by + (stepY > 0 ? 1 : 0)) - oy) / dy) : INFINITY;
    f32 tMaxZ = (dz != 0) ? (((bz + (stepZ > 0 ? 1 : 0)) - oz) / dz) : INFINITY;

    // Distance along ray to travel 1 voxel
    f32 tDeltaX = (dx != 0) ? (1.0f / abs(dx)) : INFINITY;
    f32 tDeltaY = (dy != 0) ? (1.0f / abs(dy)) : INFINITY;
    f32 tDeltaZ = (dz != 0) ? (1.0f / abs(dz)) : INFINITY;

    // Last face direction
    i32 lastFaceX = 0, lastFaceY = 0, lastFaceZ = 0;

    // Walk the ray
    for (i32 i = 0; i < 200; ++i) {
        // Check current block
        BlockType block = GetBlockAt(world, bx, by, bz);
        if (block != BlockType::Air && block != BlockType::Water) {
            result.hit = true;
            result.blockX = bx;
            result.blockY = by;
            result.blockZ = bz;

            // Determine face
            if (lastFaceX > 0) result.hitFace = BlockFace::Right;
            else if (lastFaceX < 0) result.hitFace = BlockFace::Left;
            else if (lastFaceY > 0) result.hitFace = BlockFace::Top;
            else if (lastFaceY < 0) result.hitFace = BlockFace::Bottom;
            else if (lastFaceZ > 0) result.hitFace = BlockFace::Front;
            else if (lastFaceZ < 0) result.hitFace = BlockFace::Back;

            // Calculate place position
            result.placeX = bx + lastFaceX;
            result.placeY = by + lastFaceY;
            result.placeZ = bz + lastFaceZ;

            result.distance = sqrtf((bx + 0.5f - origin.x) * (bx + 0.5f - origin.x) +
                                     (by + 0.5f - origin.y) * (by + 0.5f - origin.y) +
                                     (bz + 0.5f - origin.z) * (bz + 0.5f - origin.z));
            break;
        }

        // Check if we've exceeded max distance
        f32 dist = sqrtf((bx + 0.5f - origin.x) * (bx + 0.5f - origin.x) +
                          (by + 0.5f - origin.y) * (by + 0.5f - origin.y) +
                          (bz + 0.5f - origin.z) * (bz + 0.5f - origin.z));
        if (dist > maxDistance) break;

        // Step to next voxel
        if (tMaxX < tMaxY) {
            if (tMaxX < tMaxZ) {
                bx += stepX;
                tMaxX += tDeltaX;
                lastFaceX = stepX; lastFaceY = 0; lastFaceZ = 0;
            } else {
                bz += stepZ;
                tMaxZ += tDeltaZ;
                lastFaceX = 0; lastFaceY = 0; lastFaceZ = stepZ;
            }
        } else {
            if (tMaxY < tMaxZ) {
                by += stepY;
                tMaxY += tDeltaY;
                lastFaceX = 0; lastFaceY = stepY; lastFaceZ = 0;
            } else {
                bz += stepZ;
                tMaxZ += tDeltaZ;
                lastFaceX = 0; lastFaceY = 0; lastFaceZ = stepZ;
            }
        }
    }

    return result;
}

} // namespace painkiller
