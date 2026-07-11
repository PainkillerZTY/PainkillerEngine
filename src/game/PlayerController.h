#pragma once

#include "Types.h"
#include "Vector.h"
#include "Matrix.h"
#include "Engine.h"

namespace nebula {

// ============================================================
// PlayerController: First-person movement & camera
// ============================================================
class PlayerController {
public:
    PlayerController();
    ~PlayerController() = default;

    void Initialize(Camera* camera);
    void Update(Engine* engine, f32 deltaTime);

    // Position
    Vec3 GetPosition() const { return m_position; }
    void SetPosition(const Vec3& pos) { m_position = pos; }

    // Camera
    Camera* GetCamera() const { return m_camera; }

    // Movement flags
    bool IsOnGround() const { return m_onGround; }
    bool IsMoving() const { return m_isMoving; }

    // Physics
    Vec3 GetVelocity() const { return m_velocity; }

    // Collision helpers
    f32 GetHeight() const { return 1.8f; }  // Eye height
    f32 GetWidth() const { return 0.6f; }   // Player width

private:
    void UpdateLook(Input* input, f32 deltaTime);
    void UpdateMovement(Input* input, f32 deltaTime);

    Camera* m_camera = nullptr;

    Vec3 m_position = Vec3(0.0f, 40.0f, 0.0f); // Start position
    Vec3 m_velocity = Vec3(0.0f);

    // Look angles
    f32 m_yaw = 0.0f;
    f32 m_pitch = 0.0f;

    // Movement state
    bool m_onGround = false;
    bool m_isMoving = false;

    // Constants
    static constexpr f32 kMoveSpeed = 5.0f;
    static constexpr f32 kJumpSpeed = 8.0f;
    static constexpr f32 kGravity = -20.0f;
    static constexpr f32 kMouseSensitivity = 0.002f;
};

} // namespace nebula
