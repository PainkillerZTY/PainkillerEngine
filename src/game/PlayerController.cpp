#include "PlayerController.h"
#include "Input.h"
#include "Camera.h"
#include "Math.h"
#include "World.h"
#include "Block.h"
#include <algorithm>
#include <cmath>

namespace painkiller {

PlayerController::PlayerController() {
    m_position = Vec3(0.0f, 50.0f, 0.0f);
}

void PlayerController::Initialize(Camera* camera) {
    m_camera = camera;
    if (m_camera) {
        m_camera->SetPosition(m_position);
        m_camera->SetRotation(Vec3(0.0f, 0.0f, 0.0f));
    }
}

Vec3 PlayerController::GetCollisionMin() const {
    return Vec3(m_position.x - kHalfWidth,
                m_position.y - kEyeHeight,
                m_position.z - kHalfWidth);
}

Vec3 PlayerController::GetCollisionMax() const {
    return Vec3(m_position.x + kHalfWidth,
                m_position.y + kHeadHeight,
                m_position.z + kHalfWidth);
}

 void PlayerController::Update(Engine* engine, f32 deltaTime) {
     if (!m_camera || !engine) return;
     
     Input* input = engine->GetInput();
     m_sprinting = input->IsKeyDown(VK_SHIFT) && !input->IsKeyDown(VK_CONTROL);
     m_sneaking = input->IsKeyDown(VK_CONTROL);
     
     UpdateLook(input, deltaTime);
     UpdateMovement(input, deltaTime);
     deltaTime = std::min(deltaTime, 0.05f);
     
     // Physics + collision
     if (!m_flying) m_velocity.y += kGravity * deltaTime;
     MoveAndCollide(deltaTime);
     
     // View bob
     f32 bobAmplitude = 0.0f;
     f32 bobSpeed = 0.0f;
     if (m_sprinting) { bobAmplitude = kSprintBobAmplitude; bobSpeed = kSprintBobSpeed; }
     else if (m_sneaking) { bobAmplitude = kSneakBobAmplitude; bobSpeed = kBobSpeed * 0.5f; }
     else if (m_isMoving) { bobAmplitude = kBobAmplitude; bobSpeed = kBobSpeed; }
     
     Vec3 camPos = m_position;
     if (bobAmplitude > 0.0f && m_onGround) {
         m_bobTimer += deltaTime * bobSpeed;
         f32 bobX = sinf(m_bobTimer) * bobAmplitude;
         f32 bobY = fabsf(sinf(m_bobTimer * 2.0f)) * bobAmplitude * 0.5f;
         camPos.x += bobX;
         camPos.y += bobY;
     } else {
         m_bobTimer = 0.0f;
     }
     
     m_camera->SetPosition(camPos);
 }

void PlayerController::UpdateLook(Input* input, f32 deltaTime) {
    (void)deltaTime;
    m_yaw   -= (f32)input->GetMouseDeltaX() * kMouseSensitivity;
    m_pitch -= (f32)input->GetMouseDeltaY() * kMouseSensitivity;
    m_pitch = std::max(-1.57f, std::min(1.57f, m_pitch));
    m_camera->SetRotation(Vec3(Degrees(m_pitch), Degrees(m_yaw), 0.0f));
}

 void PlayerController::UpdateMovement(Input* input, f32 deltaTime) {
     // Toggle flight with F key
     if (input->IsKeyPressed('F')) {
         m_flying = !m_flying;
         if (!m_flying) m_onGround = false;
     }
     Vec3 moveDir(0.0f);
     if (input->IsKeyDown('W')) moveDir.z -= 1.0f;
     if (input->IsKeyDown('S')) moveDir.z += 1.0f;
     if (input->IsKeyDown('A')) moveDir.x -= 1.0f;
     if (input->IsKeyDown('D')) moveDir.x += 1.0f;
     
     if (m_flying) {
         if (input->IsKeyDown(VK_SPACE)) m_velocity.y = kFlightSpeed;
         else if (input->IsKeyDown(VK_SHIFT) || input->IsKeyDown(VK_CONTROL)) m_velocity.y = -kFlightSpeed;
         else m_velocity.y *= (1.0f - std::min(1.0f, kDeceleration * deltaTime));
     } else if (input->IsKeyPressed(VK_SPACE) && m_onGround) {
         m_velocity.y = kJumpSpeed;
         m_onGround = false;
     }
     
     m_isMoving = (moveDir.x != 0.0f || moveDir.z != 0.0f || (m_flying && fabsf(m_velocity.y) > 0.1f));
     
     if (m_isMoving) {
         f32 len = sqrtf(moveDir.x * moveDir.x + moveDir.z * moveDir.z);
         if (len > 0.0f) { moveDir.x /= len; moveDir.z /= len; }
         f32 sy = sinf(m_yaw), cy = cosf(m_yaw);
         
         f32 targetSpeed = kMoveSpeed;
         f32 accel = kAcceleration;
         if (m_sprinting) { targetSpeed = kSprintSpeed; accel = kSprintAccel; }
         if (m_sneaking) { targetSpeed = kSneakSpeed; }
         
         Vec3 targetVel;
         targetVel.x = (moveDir.x * cy + moveDir.z * sy) * targetSpeed;
         targetVel.z = (-moveDir.x * sy + moveDir.z * cy) * targetSpeed;
         
         // Smooth acceleration toward target velocity
         f32 t = std::min(1.0f, accel * deltaTime);
         m_velocity.x += (targetVel.x - m_velocity.x) * t;
         m_velocity.z += (targetVel.z - m_velocity.z) * t;
     } else {
         // Smooth deceleration
         f32 t = std::min(1.0f, kDeceleration * deltaTime);
         m_velocity.x *= (1.0f - t);
         m_velocity.z *= (1.0f - t);
     }
 }

// ============================================================
// AABB Collision Detection & Response
// ============================================================
bool PlayerController::CollideWithWorld(f32 deltaTime) {
    (void)deltaTime;
    // This helper is now integrated into MoveAndCollide
    return true;
}

void PlayerController::MoveAndCollide(f32 deltaTime) {
    if (!m_world) {
        // Fallback: no world, just simple movement
        m_position += m_velocity * deltaTime;
        if (m_position.y < 1.0f) {
            m_position.y = 1.0f;
            m_velocity.y = 0.0f;
            m_onGround = true;
        } else {
            m_onGround = false;
        }
        return;
    }

    m_onGround = false;

    // --- Y Axis (vertical, handle gravity first) ---
    {
        f32 newY = m_position.y + m_velocity.y * deltaTime;
        Vec3 playerMin = GetCollisionMin();
        Vec3 playerMax = GetCollisionMax();
        playerMin.y = newY - kEyeHeight;
        playerMax.y = newY + kHeadHeight;

        i32 minBX = (i32)floorf(playerMin.x);
        i32 maxBX = (i32)floorf(playerMax.x);
        i32 minBY = (i32)floorf(playerMin.y);
        i32 maxBY = (i32)floorf(playerMax.y);
        i32 minBZ = (i32)floorf(playerMin.z);
        i32 maxBZ = (i32)floorf(playerMax.z);

        bool collidedY = false;
        if (m_velocity.y > 0) {
            for (i32 by = maxBY; by >= minBY && !collidedY; --by) {
                for (i32 bz = minBZ; bz <= maxBZ && !collidedY; ++bz) {
                    for (i32 bx = minBX; bx <= maxBX && !collidedY; ++bx) {
                        BlockType block = m_world->GetBlock(bx, by, bz);
                        if (BlockInfo::IsOpaque(block)) {
                            newY = (f32)by - kHeadHeight;
                            m_velocity.y = 0.0f;
                            collidedY = true;
                        }
                    }
                }
            }
        } else {
            for (i32 by = minBY; by <= maxBY && !collidedY; ++by) {
                for (i32 bz = minBZ; bz <= maxBZ && !collidedY; ++bz) {
                    for (i32 bx = minBX; bx <= maxBX && !collidedY; ++bx) {
                        BlockType block = m_world->GetBlock(bx, by, bz);
                        if (BlockInfo::IsOpaque(block)) {
                            newY = (f32)(by + 1) + kEyeHeight;
                            m_velocity.y = 0.0f;
                            m_onGround = true;
                            collidedY = true;
                        }
                    }
                }
            }
        }
        m_position.y = newY;
    }

    // --- X Axis ---
    {
        f32 newX = m_position.x + m_velocity.x * deltaTime;
        Vec3 playerMin = GetCollisionMin();
        Vec3 playerMax = GetCollisionMax();
        playerMin.x = newX - kHalfWidth;
        playerMax.x = newX + kHalfWidth;

        i32 minBX = (i32)floorf(playerMin.x);
        i32 maxBX = (i32)floorf(playerMax.x);
        i32 minBY = (i32)floorf(playerMin.y);
        i32 maxBY = (i32)floorf(playerMax.y);
        i32 minBZ = (i32)floorf(playerMin.z);
        i32 maxBZ = (i32)floorf(playerMax.z);

        bool collidedX = false;
        if (m_velocity.x > 0) {
            for (i32 bx = maxBX; bx >= minBX && !collidedX; --bx) {
                for (i32 bz = minBZ; bz <= maxBZ && !collidedX; ++bz) {
                    for (i32 by = minBY; by <= maxBY && !collidedX; ++by) {
                        BlockType block = m_world->GetBlock(bx, by, bz);
                        if (BlockInfo::IsOpaque(block)) {
                            newX = (f32)bx - kHalfWidth;
                            m_velocity.x = 0.0f;
                            collidedX = true;
                        }
                    }
                }
            }
        } else {
            for (i32 bx = minBX; bx <= maxBX && !collidedX; ++bx) {
                for (i32 bz = minBZ; bz <= maxBZ && !collidedX; ++bz) {
                    for (i32 by = minBY; by <= maxBY && !collidedX; ++by) {
                        BlockType block = m_world->GetBlock(bx, by, bz);
                        if (BlockInfo::IsOpaque(block)) {
                            newX = (f32)(bx + 1) + kHalfWidth;
                            m_velocity.x = 0.0f;
                            collidedX = true;
                        }
                    }
                }
            }
        }
        m_position.x = newX;
    }

    // --- Z Axis ---
    {
        f32 newZ = m_position.z + m_velocity.z * deltaTime;
        Vec3 playerMin = GetCollisionMin();
        Vec3 playerMax = GetCollisionMax();
        playerMin.z = newZ - kHalfWidth;
        playerMax.z = newZ + kHalfWidth;

        i32 minBX = (i32)floorf(playerMin.x);
        i32 maxBX = (i32)floorf(playerMax.x);
        i32 minBY = (i32)floorf(playerMin.y);
        i32 maxBY = (i32)floorf(playerMax.y);
        i32 minBZ = (i32)floorf(playerMin.z);
        i32 maxBZ = (i32)floorf(playerMax.z);

        bool collidedZ = false;
        if (m_velocity.z > 0) {
            for (i32 bz = maxBZ; bz >= minBZ && !collidedZ; --bz) {
                for (i32 bx = minBX; bx <= maxBX && !collidedZ; ++bx) {
                    for (i32 by = minBY; by <= maxBY && !collidedZ; ++by) {
                        BlockType block = m_world->GetBlock(bx, by, bz);
                        if (BlockInfo::IsOpaque(block)) {
                            newZ = (f32)bz - kHalfWidth;
                            m_velocity.z = 0.0f;
                            collidedZ = true;
                        }
                    }
                }
            }
        } else {
            for (i32 bz = minBZ; bz <= maxBZ && !collidedZ; ++bz) {
                for (i32 bx = minBX; bx <= maxBX && !collidedZ; ++bx) {
                    for (i32 by = minBY; by <= maxBY && !collidedZ; ++by) {
                        BlockType block = m_world->GetBlock(bx, by, bz);
                        if (BlockInfo::IsOpaque(block)) {
                            newZ = (f32)(bz + 1) + kHalfWidth;
                            m_velocity.z = 0.0f;
                            collidedZ = true;
                        }
                    }
                }
            }
        }
        m_position.z = newZ;
    }
}

} // namespace painkiller
