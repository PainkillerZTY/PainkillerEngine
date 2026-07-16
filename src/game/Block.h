#pragma once

#include "Types.h"
#include "Vector.h"
#include <array>

namespace painkiller {

// ============================================================
// Block Types
// ============================================================
enum class BlockType : u8 {
    Air = 0,
    Grass,
    Dirt,
    Stone,
    Cobblestone,
    Wood,
    OakLog,
    Leaves,
    OakPlanks,
    Sand,
    Water,
    Snow,
    Bedrock,
    CoalOre,
    IronOre,
    GoldOre,
    DiamondOre,
    CraftingTable,
    Furnace,
    FurnaceLit,
    Chest,
    Glass,
    Bookshelf,
    Sticks,
    // Add more as needed
    NumTypes,
};

// ============================================================
// Block Face Directions
// ============================================================
enum class BlockFace : u8 {
    Right  = 0, // +X
    Left   = 1, // -X
    Top    = 2, // +Y
    Bottom = 3, // -Y
    Front  = 4, // +Z
    Back   = 5, // -Z
    NumFaces,
};

// ============================================================
// Block Properties
// ============================================================
struct BlockInfo {
    const char* name;
    Vec3        topColor;
    Vec3        sideColor;
    Vec3        bottomColor;
    bool        isSolid;
    bool        isTransparent;
    bool        isCube;       // Full cube or special shape
    f32         hardness;     // Break time multiplier
    bool        hasTopTexture; // Different top/bottom vs sides (grass)

    static const BlockInfo& Get(BlockType type);
    static Vec3 GetFaceColor(BlockType type, BlockFace face);
    static bool IsSolid(BlockType type);
    static bool IsTransparent(BlockType type);
    static bool IsOpaque(BlockType type);
    static f32  GetHardness(BlockType type);
};

// ============================================================
// Block Coordinate Helpers
// ============================================================
struct BlockCoord {
    i32 x, y, z;

    BlockCoord() : x(0), y(0), z(0) {}
    BlockCoord(i32 x, i32 y, i32 z) : x(x), y(y), z(z) {}

    bool operator==(const BlockCoord& o) const {
        return x == o.x && y == o.y && z == o.z;
    }
    bool operator!=(const BlockCoord& o) const {
        return !(*this == o);
    }
};

// Block position in world coordinates
using WorldBlockPos = BlockCoord;

// ============================================================
// Pre-defined block colors (simple palette for MVP)
// ============================================================
namespace BlockColors {
    const Vec3 GrassTop     = Vec3(0.29f, 0.65f, 0.14f);
    const Vec3 GrassSide    = Vec3(0.27f, 0.50f, 0.12f);
    const Vec3 Dirt         = Vec3(0.45f, 0.32f, 0.18f);
    const Vec3 Stone        = Vec3(0.50f, 0.50f, 0.50f);
    const Vec3 Cobblestone  = Vec3(0.40f, 0.40f, 0.40f);
    const Vec3 OakLogSide   = Vec3(0.42f, 0.29f, 0.13f);
    const Vec3 OakLogTop    = Vec3(0.55f, 0.37f, 0.16f);
    const Vec3 Leaves       = Vec3(0.15f, 0.50f, 0.08f);
    const Vec3 Planks       = Vec3(0.52f, 0.36f, 0.18f);
    const Vec3 Sand         = Vec3(0.76f, 0.70f, 0.50f);
    const Vec3 Water        = Vec3(0.20f, 0.40f, 0.80f);
    const Vec3 Snow         = Vec3(0.95f, 0.95f, 0.98f);
    const Vec3 Bedrock      = Vec3(0.20f, 0.20f, 0.20f);
    const Vec3 CoalOre      = Vec3(0.25f, 0.25f, 0.25f);
    const Vec3 IronOre      = Vec3(0.75f, 0.65f, 0.55f);
    const Vec3 GoldOre      = Vec3(0.85f, 0.75f, 0.20f);
    const Vec3 DiamondOre   = Vec3(0.30f, 0.80f, 0.80f);
    const Vec3 CraftingTableTop = Vec3(0.55f, 0.45f, 0.30f);
    const Vec3 CraftingTableSide = Vec3(0.45f, 0.35f, 0.22f);
    const Vec3 FurnaceTop    = Vec3(0.40f, 0.40f, 0.40f);
    const Vec3 FurnaceSide   = Vec3(0.45f, 0.40f, 0.35f);
    const Vec3 FurnaceFront  = Vec3(0.30f, 0.30f, 0.30f);
    const Vec3 Glass         = Vec3(0.60f, 0.80f, 0.90f);
    const Vec3 Bookshelf     = Vec3(0.45f, 0.32f, 0.18f);
}

} // namespace painkiller
