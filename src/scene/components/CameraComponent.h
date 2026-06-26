#pragma once

#include "Component.h"
#include "Vector.h"
#include "Matrix.h"
#include "Math.h"

namespace nebula {

// ?? Camera Component ??
struct CameraComponent {
    f32 fov        = 60.0f;     // Degrees
    f32 nearPlane   = 0.1f;
    f32 farPlane    = 100.0f;
    bool isPrimary  = true;
    f32 aspectRatio = 16.0f / 9.0f;
    
    Mat4 GetProjectionMatrix() const {
        return Perspective(fov, aspectRatio, nearPlane, farPlane);
    }
};

} // namespace nebula
