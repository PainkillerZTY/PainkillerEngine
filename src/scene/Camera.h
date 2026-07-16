#pragma once

#include "Vector.h"
#include "Matrix.h"
#include "Math.h"
#include "components/CameraComponent.h"

namespace painkiller {

class Camera {
public:
    Camera() = default;
    
    void SetPerspective(f32 fovDeg, f32 aspect, f32 nearZ, f32 farZ) {
        m_fov = fovDeg;
        m_aspect = aspect;
        m_near = nearZ;
        m_far = farZ;
        m_projectionDirty = true;
    }
    
    void SetPosition(const Vec3& position) { m_position = position; }
    void SetRotation(const Vec3& eulerDeg) { m_euler = eulerDeg; m_viewDirty = true; }
    void LookAt(const Vec3& eye, const Vec3& target) {
        m_position = eye;
        m_viewMatrix = glm::lookAt(eye, target, Vec3(0.0f, 1.0f, 0.0f));
        m_viewDirty = false;
    }
    
    const Mat4& GetViewMatrix() {
        if (m_viewDirty) {
            RecalculateView();
        }
        return m_viewMatrix;
    }
    
    const Mat4& GetProjectionMatrix() {
        if (m_projectionDirty) {
            RecalculateProjection();
        }
        return m_projectionMatrix;
    }
    
    Mat4 GetViewProjectionMatrix() {
        return GetProjectionMatrix() * GetViewMatrix();
    }
    
     Vec3 GetPosition() const { return m_position; }
     Vec3 GetForward() const;
     Vec3 GetRight() const;
     
     f32 GetYaw() const { return m_euler.y; }
     f32 GetPitch() const { return m_euler.x; }
     
     // Dynamic FOV (sprint FOV expansion, smooth transitions)
     void SetTargetFOV(f32 fovDeg) { m_targetFOV = fovDeg; }
     f32 GetTargetFOV() const { return m_targetFOV; }
     void UpdateFOV(f32 deltaTime);

private:
    void RecalculateView();
    void RecalculateProjection();
    
    Vec3 m_position = Vec3(0.0f, 0.0f, 3.0f);
    Vec3 m_euler = Vec3(0.0f); // degrees
    
     f32 m_fov = 70.0f;
     f32 m_targetFOV = 70.0f;  // 70 = default Bedrock FOV
     f32 m_aspect = 16.0f / 9.0f;
    f32 m_near = 0.1f;
    f32 m_far = 100.0f;
    
    Mat4 m_viewMatrix = Mat4(1.0f);
    Mat4 m_projectionMatrix = Mat4(1.0f);
    
    bool m_viewDirty = true;
    bool m_projectionDirty = true;
};

} // namespace painkiller

