#pragma once

#include "Component.h"
#include "Transform.h"

namespace nebula {

// ?? Transform Component ??
struct TransformComponent {
    Vec3 position   = Vec3(0.0f);
    Vec3 eulerRotation = Vec3(0.0f); // Degrees
    Vec3 scale      = Vec3(1.0f);
    
    Quat GetQuaternion() const {
        return Quat(glm::radians(eulerRotation));
    }
    
    Transform ToTransform() const {
        Transform t;
        t.position = position;
        t.rotation = GetQuaternion();
        t.scale = scale;
        return t;
    }
    
    Mat4 ToMat4() const {
        return ToTransform().ToMat4();
    }
};

} // namespace nebula
