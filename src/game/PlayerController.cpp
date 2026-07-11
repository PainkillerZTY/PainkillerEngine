#include "PlayerController.h"
#include "Input.h"
#include "Camera.h"
#include "Math.h"
#include <algorithm>

namespace nebula {

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

void PlayerController::Update(Engine* engine, f32 deltaTime) {
    if (!m_camera || !engine) return;

    Input* input = engine->GetInput();

    UpdateLook(input, deltaTime);
    UpdateMovement(input, deltaTime);

    // Clamp delta time
    deltaTime = std::min(deltaTime, 0.05f);

    // Apply gravity
    m_velocity.y += kGravity * deltaTime;

    // Simple ground collision (y < 1)
    if (m_position.y < 1.0f) {
        m_position.y = 1.0f;
        m_velocity.y = 0.0f;
        m_onGround = true;
    } else {
        m_onGround = false;
    }

    // Apply velocity
    m_position += m_velocity * deltaTime;

    // Update camera
    m_camera->SetPosition(m_position);
}

void PlayerController::UpdateLook(Input* input, f32 deltaTime) {
    (void)deltaTime;

    // Mouse look
    m_yaw   -= (f32)input->GetMouseDeltaX() * kMouseSensitivity;
    m_pitch -= (f32)input->GetMouseDeltaY() * kMouseSensitivity;

    // Clamp pitch
    m_pitch = std::max(-1.57f, std::min(1.57f, m_pitch));

    // Apply to camera
    m_camera->SetRotation(Vec3(Degrees(m_pitch), Degrees(m_yaw), 0.0f));
}

void PlayerController::UpdateMovement(Input* input, f32 deltaTime) {
    Vec3 moveDir(0.0f);

    // Forward/Backward
    if (input->IsKeyDown('W')) moveDir.z -= 1.0f;
    if (input->IsKeyDown('S')) moveDir.z += 1.0f;

    // Left/Right
    if (input->IsKeyDown('A')) moveDir.x -= 1.0f;
    if (input->IsKeyDown('D')) moveDir.x += 1.0f;

    // Jump
    if (input->IsKeyPressed(VK_SPACE) && m_onGround) {
        m_velocity.y = kJumpSpeed;
        m_onGround = false;
    }

    m_isMoving = (moveDir.x != 0.0f || moveDir.z != 0.0f);

    if (m_isMoving) {
        // Normalize
        f32 len = sqrtf(moveDir.x * moveDir.x + moveDir.z * moveDir.z);
        if (len > 0.0f) {
            moveDir.x /= len;
            moveDir.z /= len;
        }

        // Rotate movement direction by yaw
        f32 sinYaw = sinf(m_yaw);
        f32 cosYaw = cosf(m_yaw);

        Vec3 worldMove;
        worldMove.x = moveDir.x * cosYaw - moveDir.z * sinYaw;
        worldMove.z = moveDir.x * sinYaw + moveDir.z * cosYaw;

        m_velocity.x = worldMove.x * kMoveSpeed;
        m_velocity.z = worldMove.z * kMoveSpeed;
    } else {
        // Friction
        m_velocity.x *= 0.8f;
        m_velocity.z *= 0.8f;
    }
}

} // namespace nebula
