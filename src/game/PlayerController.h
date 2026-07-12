#pragma once

#include "Types.h"
#include "Vector.h"
#include "Matrix.h"
#include "Engine.h"

namespace painkiller {

class World;

// ============================================================
// PlayerController: First-person movement & camera
// ============================================================
class PlayerController {
public:
    PlayerController();
    ~PlayerController() = default;

    void Initialize(Camera* camera);
    void SetWorld(World* world) { m_world = world; }
    void Update(Engine* engine, f32 deltaTime);

    // Position
    Vec3 GetPosition() const { return m_position; }
     void SetPosition(const Vec3& pos) { m_position = pos; }
     f32 GetSprintFrac() const {
         f32 speed = sqrtf(m_velocity.x * m_velocity.x + m_velocity.z * m_velocity.z);
         f32 maxSpeed = m_sprinting ? kSprintSpeed : (m_sneaking ? kSneakSpeed : kMoveSpeed);
         return maxSpeed > 0.01f ? std::min(1.0f, speed / maxSpeed) : 0.0f;
     }

    // Camera
    Camera* GetCamera() const { return m_camera; }

    // Collision & movement
    bool IsOnGround() const { return m_onGround; }
     bool IsMoving() const { return m_isMoving; }
     bool IsSprinting() const { return m_sprinting; }
     bool IsSneaking() const { return m_sneaking; }
     bool IsFlying() const { return m_flying; }
    Vec3 GetCollisionMin() const;
    Vec3 GetCollisionMax() const;

    // Physics
    Vec3 GetVelocity() const { return m_velocity; }

    // Collision helpers
    f32 GetHeight() const { return 1.8f; }  // Eye height
    f32 GetWidth() const { return 0.6f; }   // Player width

private:
    void UpdateLook(Input* input, f32 deltaTime);
    void UpdateMovement(Input* input, f32 deltaTime);
    void MoveAndCollide(f32 deltaTime);
    bool CollideWithWorld(f32 deltaTime);

    Camera* m_camera = nullptr;
    World* m_world = nullptr;

    Vec3 m_position = Vec3(0.0f, 40.0f, 0.0f); // Start position
    Vec3 m_velocity = Vec3(0.0f);

     f32 m_yaw = 0.0f;
     f32 m_pitch = 0.0f;
     
     f32 m_bobTimer = 0.0f;
     bool m_sprinting = false;
     bool m_sneaking = false;
     bool m_flying = false;

    // Movement state
    bool m_onGround = false;
    bool m_isMoving = false;

     static constexpr f32 kMoveSpeed = 4.317f;      // ~4.317 m/s walk (Minecraft-like)
     static constexpr f32 kSprintSpeed = 5.612f;     // ~5.612 m/s sprint
     static constexpr f32 kSneakSpeed = 1.295f;
     static constexpr f32 kFlightSpeed = 10.0f;
     static constexpr f32 kFlightAccel = 20.0f;      // ~1.295 m/s sneak
     static constexpr f32 kJumpSpeed = 8.0f;
     static constexpr f32 kGravity = -20.0f;
     static constexpr f32 kMouseSensitivity = 0.002f;
     static constexpr f32 kAcceleration = 12.0f;     // How fast we reach target speed
     static constexpr f32 kSprintAccel = 8.0f;       // Slightly slower accel when sprinting
     static constexpr f32 kDeceleration = 10.0f;     // How fast we slow down
     static constexpr f32 kBobAmplitude = 0.04f;     // View bob height
     static constexpr f32 kBobSpeed = 10.0f;
     static constexpr f32 kSprintBobAmplitude = 0.08f;
     static constexpr f32 kSprintBobSpeed = 14.0f;
     static constexpr f32 kSneakBobAmplitude = 0.02f;
     static constexpr f32 kEyeHeight = 1.6f;
     static constexpr f32 kHeadHeight = 0.2f;
     static constexpr f32 kHalfWidth = 0.3f;
     static constexpr f32 kSneakEyeHeight = 1.4f;
};

} // namespace painkiller
