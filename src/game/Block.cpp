#include "Block.h"

namespace painkiller {

// ============================================================
// BlockInfo table
// ============================================================
static const BlockInfo kBlockInfo[] = {
    {"Air",       Vec3(0),      Vec3(0),      Vec3(0),      false, true,  true,  0.0f, false}, // 0: Air
    {"Grass",     BlockColors::GrassTop,  BlockColors::GrassSide, BlockColors::Dirt,  true, false, true,  0.6f, true }, // 1: Grass
    {"Dirt",      BlockColors::Dirt,      BlockColors::Dirt,     BlockColors::Dirt,   true, false, true,  0.5f, false}, // 2: Dirt
    {"Stone",     BlockColors::Stone,     BlockColors::Stone,    BlockColors::Stone,  true, false, true,  1.5f, false}, // 3: Stone
    {"Cobblestone", BlockColors::Cobblestone, BlockColors::Cobblestone, BlockColors::Cobblestone, true, false, true, 2.0f, false}, // 4
    {"Wood",      Vec3(0.42f,0.29f,0.13f), Vec3(0.42f,0.29f,0.13f), Vec3(0.42f,0.29f,0.13f), true, false, true, 1.5f, false}, // 5
    {"OakLog",    BlockColors::OakLogTop, BlockColors::OakLogSide, BlockColors::OakLogTop, true, false, true, 1.5f, true}, // 6
    {"Leaves",    BlockColors::Leaves,    BlockColors::Leaves,   BlockColors::Leaves, true, true,  true,  0.2f, false}, // 7
    {"OakPlanks", BlockColors::Planks,    BlockColors::Planks,   BlockColors::Planks, true, false, true,  1.5f, false}, // 8
    {"Sand",      BlockColors::Sand,      BlockColors::Sand,     BlockColors::Sand,   true, false, true,  0.5f, false}, // 9
    {"Water",     BlockColors::Water,     BlockColors::Water,    BlockColors::Water,  false, true,  true,  0.0f, false}, // 10
    {"Snow",      BlockColors::Snow,      BlockColors::Snow,     BlockColors::Snow,   true, false, true,  0.2f, false}, // 11
        {"Bedrock",   BlockColors::Bedrock,   BlockColors::Bedrock,  BlockColors::Bedrock, true, false, true,  -1.0f, false},
    {"CoalOre",    Vec3(0.20f,0.20f,0.20f), Vec3(0.20f,0.20f,0.20f), Vec3(0.20f,0.20f,0.20f), true, false, true,  3.0f, false},
    {"IronOre",    Vec3(0.75f,0.60f,0.50f), Vec3(0.75f,0.60f,0.50f), Vec3(0.75f,0.60f,0.50f), true, false, true,  4.0f, false},
    {"GoldOre",    Vec3(0.85f,0.70f,0.15f), Vec3(0.85f,0.70f,0.15f), Vec3(0.85f,0.70f,0.15f), true, false, true,  5.0f, false},
    {"DiamondOre", Vec3(0.20f,0.70f,0.75f), Vec3(0.20f,0.70f,0.75f), Vec3(0.20f,0.70f,0.75f), true, false, true,  6.0f, false},
    {"CraftingTable", Vec3(0.55f,0.45f,0.30f), Vec3(0.45f,0.35f,0.22f), Vec3(0.40f,0.40f,0.40f), true, false, true, 2.0f, true},
    {"Furnace",    Vec3(0.45f,0.40f,0.35f), Vec3(0.45f,0.40f,0.35f), Vec3(0.40f,0.40f,0.40f), true, false, true, 3.5f, false},
    {"FurnaceLit", Vec3(1.00f,0.60f,0.20f), Vec3(0.45f,0.40f,0.35f), Vec3(0.40f,0.40f,0.40f), true, false, true, 3.5f, false},
    {"Chest",      Vec3(0.50f,0.35f,0.15f), Vec3(0.50f,0.35f,0.15f), Vec3(0.50f,0.35f,0.15f), true, false, true, 2.5f, false},
    {"Glass",      Vec3(0.60f,0.80f,0.90f), Vec3(0.60f,0.80f,0.90f), Vec3(0.60f,0.80f,0.90f), false, true, true,  0.3f, false},
    {"Sticks",     Vec3(0.52f,0.36f,0.18f), Vec3(0.52f,0.36f,0.18f),Vec3(0.52f,0.36f,0.18f), false, true, true, 0.0f, false},
    {"Bookshelf",  Vec3(0.50f,0.35f,0.20f), Vec3(0.50f,0.35f,0.20f), Vec3(0.50f,0.35f,0.20f), true, false, true, 1.5f, false},
};


static constexpr i32 kNumBlockTypes = sizeof(kBlockInfo) / sizeof(kBlockInfo[0]);

const BlockInfo& BlockInfo::Get(BlockType type) {
    i32 idx = (i32)type;
    if (idx < 0 || idx >= kNumBlockTypes) {
        // Return air as default
        return kBlockInfo[0];
    }
    return kBlockInfo[idx];
}

Vec3 BlockInfo::GetFaceColor(BlockType type, BlockFace face) {
    const BlockInfo& info = Get(type);
    switch (face) {
        case BlockFace::Top:    return info.topColor;
        case BlockFace::Bottom: return info.bottomColor;
        default:                return info.sideColor;
    }
}

bool BlockInfo::IsSolid(BlockType type) {
    return Get(type).isSolid;
}

bool BlockInfo::IsTransparent(BlockType type) {
    return Get(type).isTransparent;
}

bool BlockInfo::IsOpaque(BlockType type) {
    return IsSolid(type) && !IsTransparent(type);
}

f32 BlockInfo::GetHardness(BlockType type) {
    return Get(type).hardness;
}

} // namespace painkiller
