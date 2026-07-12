#pragma once

#include "Component.h"
#include "Vector.h"

namespace painkiller {

// ?? Material Component ??
struct MaterialComponent {
    Vec3 albedo         = Vec3(1.0f, 1.0f, 1.0f);
    f32 metallic        = 0.0f;
    f32 roughness       = 0.5f;
    f32 ao              = 1.0f;
    
    u32 albedoTexture   = kInvalidHandle;
    u32 normalTexture   = kInvalidHandle;
    u32 metallicTexture = kInvalidHandle;
    u32 roughnessTexture = kInvalidHandle;
};

} // namespace painkiller
