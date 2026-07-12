 #include "Camera.h"
 #include "Math.h"
 #include <cmath>

namespace painkiller {

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

 
 void Camera::UpdateFOV(f32 deltaTime) {
     if (std::abs(m_fov - m_targetFOV) < 0.01f) {
         m_fov = m_targetFOV;
         return;
     }
     m_fov += (m_targetFOV - m_fov) * std::min(1.0f, 8.0f * deltaTime);
     m_projectionDirty = true;
 }
 
 } // namespace painkiller
