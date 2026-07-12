#pragma once

#include "Types.h"
#include <string>

namespace painkiller {

// ?? Base Component ??
// Components are plain data structs.
// We use simple uint32_t IDs for entities.
using EntityID = u32;
constexpr EntityID kInvalidEntity = EntityID(-1);

// ?? Component Types ??
enum class ComponentType : u8 {
    Transform,
    Mesh,
    Camera,
    Light,
    Material,
    Count,
};

} // namespace painkiller
