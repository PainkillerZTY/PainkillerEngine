#include "Input.h"

namespace nebula {

void Input::BeginFrame() {
    m_prevKeys = m_keys;
    m_prevMouseButtons = m_mouseButtons;
    m_mouseDeltaX = 0;
    m_mouseDeltaY = 0;
    m_scrollDelta = 0;
}

void Input::OnKeyDown(u32 key) {
    if (key < kMaxKeys) m_keys[key] = true;
}

void Input::OnKeyUp(u32 key) {
    if (key < kMaxKeys) m_keys[key] = false;
}

void Input::OnMouseMove(i32 x, i32 y) {
    m_mouseDeltaX = x - m_mouseX;
    m_mouseDeltaY = y - m_mouseY;
    m_mouseX = x;
    m_mouseY = y;
}

void Input::OnMouseDown(u32 button) {
    if (button < kMaxMouseButtons) m_mouseButtons[button] = true;
}

void Input::OnMouseUp(u32 button) {
    if (button < kMaxMouseButtons) m_mouseButtons[button] = false;
}

void Input::OnScroll(i32 delta) {
    m_scrollDelta = delta;
}

} // namespace nebula
