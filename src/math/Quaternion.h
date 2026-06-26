#pragma once

#include "Vector.h"
#include <glm/gtc/quaternion.hpp>

namespace nebula {

// Quat is defined in Vector.h as glm::quat

inline Quat QuatIdentity() { return Quat(1.0f, 0.0f, 0.0f, 0.0f); }

inline Quat AngleAxis(f32 angleDeg, const Vec3& axis) {
    return glm::angleAxis(glm::radians(angleDeg), axis);
}

inline Quat Euler(const Vec3& eulerDeg) {
    return glm::quat(glm::radians(eulerDeg));
}

inline Vec3 EulerAngles(const Quat& q) {
    return glm::degrees(glm::eulerAngles(q));
}

inline Quat Normalize(const Quat& q) { return glm::normalize(q); }

inline Quat Slerp(const Quat& a, const Quat& b, f32 t) {
    return glm::slerp(a, b, t);
}

inline Mat4 ToMat4(const Quat& q) { return glm::mat4_cast(q); }

} // namespace nebula
