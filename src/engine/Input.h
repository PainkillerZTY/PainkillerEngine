// ============================================================
// Input.h — Keyboard + mouse input manager
//
// Tracks 256 keyboard keys, 8 mouse buttons per frame.
// Supports raw mouse deltas (WM_INPUT) for FPS controls.
// BeginFrame() copies current state to previous for edge detection.
// ============================================================
#pragma once

#include "Types.h"
#include <array>

namespace painkiller {

// ?? Input Manager ??
class Input {
public:
    Input() = default;
    
    // ?? Keyboard ??
    bool IsKeyDown(u32 key) const { return m_keys[key]; }
    bool IsKeyPressed(u32 key) const { return m_keys[key] && !m_prevKeys[key]; }
    bool IsKeyReleased(u32 key) const { return !m_keys[key] && m_prevKeys[key]; }
    
    // ?? Mouse ??
    bool IsMouseDown(u32 button) const { return m_mouseButtons[button]; }
    bool IsMousePressed(u32 button) const { return m_mouseButtons[button] && !m_prevMouseButtons[button]; }
    
    i32 GetMouseX() const { return m_mouseX; }
    i32 GetMouseY() const { return m_mouseY; }
     // Prefer raw input deltas (hardware-level, no cursor-warp pollution)
     i32 GetMouseDeltaX() const { return m_rawDeltaX != 0 ? m_rawDeltaX : m_mouseDeltaX; }
     i32 GetMouseDeltaY() const { return m_rawDeltaY != 0 ? m_rawDeltaY : m_mouseDeltaY; }
    i32 GetScrollDelta() const { return m_scrollDelta; }
    
    // ?? Update (called by Engine) ??
    void BeginFrame();
    void OnKeyDown(u32 key);
    void OnKeyUp(u32 key);
    void OnMouseMove(i32 x, i32 y);
    void OnMouseDown(u32 button);
    void OnMouseUp(u32 button);
     void OnScroll(i32 delta);
     
     // Reset tracking position without generating a delta (used after cursor warp)
     void ResetMousePosition(i32 x, i32 y) { m_mouseX = x; m_mouseY = y; m_mouseDeltaX = 0; m_mouseDeltaY = 0; }
     
     // Raw input (hardware mouse deltas, from WM_INPUT)
     void OnRawMouseDelta(i32 dx, i32 dy) { m_rawDeltaX += dx; m_rawDeltaY += dy; }

private:
    static constexpr u32 kMaxKeys = 256;
    static constexpr u32 kMaxMouseButtons = 8;
    
    std::array<bool, kMaxKeys> m_keys = {};
    std::array<bool, kMaxKeys> m_prevKeys = {};
    
    std::array<bool, kMaxMouseButtons> m_mouseButtons = {};
    std::array<bool, kMaxMouseButtons> m_prevMouseButtons = {};
    
    i32 m_mouseX = 0;
    i32 m_mouseY = 0;
     i32 m_mouseDeltaX = 0;
     i32 m_mouseDeltaY = 0;
     i32 m_rawDeltaX = 0;
     i32 m_rawDeltaY = 0;
     i32 m_scrollDelta = 0;
};

} // namespace painkiller
