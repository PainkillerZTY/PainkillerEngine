#pragma once

#include "Vector.h"
#include "Matrix.h"
#include "Quaternion.h"

namespace painkiller {

struct Transform {
    Vec3 position = Vec3(0.0f);
    Quat rotation = QuatIdentity();
    Vec3 scale    = Vec3(1.0f);

    Mat4 ToMat4() const {
        Mat4 m = glm::translate(Mat4Identity(), position);
        m = m * glm::mat4_cast(rotation);
        m = glm::scale(m, scale);
        return m;
    }

    Vec3 Forward() const {
        return glm::normalize(rotation * Vec3(0.0f, 0.0f, -1.0f));
    }

    Vec3 Right() const {
        return glm::normalize(rotation * Vec3(1.0f, 0.0f, 0.0f));
    }

    Vec3 Up() const {
        return glm::normalize(rotation * Vec3(0.0f, 1.0f, 0.0f));
    }
};

// ?? Common Transform Helpers ??
inline Transform TransformIdentity() {
    return Transform{};
}

} // namespace painkiller
