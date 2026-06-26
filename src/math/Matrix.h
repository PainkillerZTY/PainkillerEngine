#pragma once

#include "Vector.h"

namespace nebula {

// ?? Perspective / View Matrix Helpers ??
inline Mat4 Perspective(f32 fovY, f32 aspect, f32 nearZ, f32 farZ) {
    return glm::perspective(glm::radians(fovY), aspect, nearZ, farZ);
}

inline Mat4 LookAt(const Vec3& eye, const Vec3& center, const Vec3& up) {
    return glm::lookAt(eye, center, up);
}

inline Mat4 Orthographic(f32 left, f32 right, f32 bottom, f32 top, f32 nearZ, f32 farZ) {
    return glm::ortho(left, right, bottom, top, nearZ, farZ);
}

// ?? Translation / Rotation / Scale ??
inline Mat4 Translate(const Mat4& m, const Vec3& v) {
    return glm::translate(m, v);
}

inline Mat4 Rotate(const Mat4& m, f32 angle, const Vec3& axis) {
    return glm::rotate(m, glm::radians(angle), axis);
}

inline Mat4 Scale(const Mat4& m, const Vec3& v) {
    return glm::scale(m, v);
}

// ?? Matrix Access ??
inline const f32* ValuePtr(const Mat4& m) {
    return glm::value_ptr(m);
}

inline const f32* ValuePtr(const Vec3& v) {
    return glm::value_ptr(v);
}

// ?? Identity ??
inline Mat4 Mat4Identity() { return Mat4(1.0f); }
inline Mat3 Mat3Identity() { return Mat3(1.0f); }

// ?? Transpose / Inverse ??
inline Mat4 Transpose(const Mat4& m) { return glm::transpose(m); }
inline Mat4 Inverse(const Mat4& m) { return glm::inverse(m); }

} // namespace nebula
