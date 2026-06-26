#include "Camera.h"
#include "Math.h"

namespace nebula {

void Camera::RecalculateView() {
    Quat rot = Quat(glm::radians(m_euler));
    Vec3 forward = rot * Vec3(0.0f, 0.0f, -1.0f);
    Vec3 target = m_position + forward;
    m_viewMatrix = glm::lookAt(m_position, target, Vec3(0.0f, 1.0f, 0.0f));
    m_viewDirty = false;
}

void Camera::RecalculateProjection() {
    m_projectionMatrix = Perspective(m_fov, m_aspect, m_near, m_far);
    m_projectionDirty = false;
}

Vec3 Camera::GetForward() const {
    Quat rot = Quat(glm::radians(m_euler));
    return glm::normalize(rot * Vec3(0.0f, 0.0f, -1.0f));
}

Vec3 Camera::GetRight() const {
    Quat rot = Quat(glm::radians(m_euler));
    return glm::normalize(rot * Vec3(1.0f, 0.0f, 0.0f));
}

} // namespace nebula
