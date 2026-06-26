#pragma once

#include "Types.h"
#include <array>

namespace nebula {

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
    i32 GetMouseDeltaX() const { return m_mouseDeltaX; }
    i32 GetMouseDeltaY() const { return m_mouseDeltaY; }
    i32 GetScrollDelta() const { return m_scrollDelta; }
    
    // ?? Update (called by Engine) ??
    void BeginFrame();
    void OnKeyDown(u32 key);
    void OnKeyUp(u32 key);
    void OnMouseMove(i32 x, i32 y);
    void OnMouseDown(u32 button);
    void OnMouseUp(u32 button);
    void OnScroll(i32 delta);
    
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
    i32 m_scrollDelta = 0;
};

} // namespace nebula
