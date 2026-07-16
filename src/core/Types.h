#pragma once

#include <cstdint>
#include <cstddef>
#include <memory>
#include <string>
#include <vector>
#include <array>
#include <unordered_map>
#include <functional>
#include <cassert>

namespace painkiller {

// ?? Integer Types ??
using i8  = int8_t;
using i16 = int16_t;
using i32 = int32_t;
using i64 = int64_t;

using u8  = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
using u64 = uint64_t;

using f32 = float;
using f64 = double;

using usize = size_t;

// ?? Handle Types ??
using Handle = u32;
constexpr Handle kInvalidHandle = Handle(-1);

enum class BufferType : u8 {
    Vertex,
    Index,
    Constant,
    Structured,
};

enum class TextureType : u8 {
    Texture2D,
    TextureCube,
};

enum class ShaderStage : u8 {
    Vertex,
    Fragment,
    Geometry,
    Compute,
};

// ?? Resource Usage ??
enum class ResourceUsage : u8 {
    Static,    // Upload once, rarely changed
    Dynamic,   // Updated per-frame
    Immutable, // Created and never changed
};

// ?? Render Backend ??
enum class RenderBackend : u8 {
    OpenGL,
    DirectX11,
    Vulkan,
};

} // namespace painkiller
