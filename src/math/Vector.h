#pragma once

#include "Types.h"

// GLM includes - disable constexpr for GCC 6.3 compatibility
#define GLM_FORCE_NO_CONSTEXPR
#define GLM_FORCE_NO_CTOR_INIT
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace painkiller {

using Vec2 = glm::vec2;
using Vec2i = glm::ivec2;
using Vec2u = glm::uvec2;
using Vec3 = glm::vec3;
using Vec3i = glm::ivec3;
using Vec4 = glm::vec4;
using Mat3 = glm::mat3;
using Mat4 = glm::mat4;
using Quat = glm::quat;

constexpr f32 kPi       = 3.14159265358979323846f;
constexpr f32 kHalfPi   = 1.57079632679489661923f;
constexpr f32 kTwoPi    = 6.28318530717958647692f;
constexpr f32 kEpsilon  = 1.192092896e-07f;

inline f32 Radians(f32 degrees) { return glm::radians(degrees); }
inline f32 Degrees(f32 radians) { return glm::degrees(radians); }
inline f32 Lerp(f32 a, f32 b, f32 t) { return a + (b - a) * t; }
inline Vec3 Lerp(const Vec3& a, const Vec3& b, f32 t) { return a + (b - a) * t; }
inline f32 Clamp(f32 v, f32 min, f32 max) { return glm::clamp(v, min, max); }
inline f32 Saturate(f32 v) { return glm::clamp(v, 0.0f, 1.0f); }

struct Ray {
    Vec3 origin;
    Vec3 direction;
};

struct BoundingBox {
    Vec3 min;
    Vec3 max;
    bool Contains(const Vec3& point) const {
        return point.x >= min.x && point.x <= max.x &&
               point.y >= min.y && point.y <= max.y &&
               point.z >= min.z && point.z <= max.z;
    }
};

} // namespace painkiller
