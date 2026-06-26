#pragma once

#include "Component.h"
#include "RenderTypes.h"

namespace nebula {

// ?? Light Component ??
struct LightComponent {
    LightType type      = LightType::Directional;
    Vec3 color          = Vec3(1.0f, 1.0f, 1.0f);
    f32 intensity       = 1.0f;
    f32 range           = 10.0f;
    f32 spotAngle       = 45.0f; // Degrees, for spot lights
    bool castsShadows   = false;
};

} // namespace nebula
