#include "Timer.h"

namespace nebula {

Timer::Timer() {
    m_startTime = std::chrono::high_resolution_clock::now();
    m_lastFrameTime = m_startTime;
}

void Timer::Tick() {
    auto now = std::chrono::high_resolution_clock::now();
    
    // Delta time in seconds
    std::chrono::duration<f32> delta = now - m_lastFrameTime;
    m_deltaTime = delta.count();
    
    // Clamp delta time to avoid huge jumps
    if (m_deltaTime > 0.1f) m_deltaTime = 0.1f;
    
    m_totalTime += m_deltaTime;
    m_lastFrameTime = now;
    m_frameCount++;
    
    // FPS calculation
    m_fpsAccumulator += m_deltaTime;
    m_fpsFrameCount++;
    if (m_fpsFrameCount >= kFPSUpdateInterval) {
        m_fps = (f32)m_fpsFrameCount / m_fpsAccumulator;
        m_fpsAccumulator = 0.0f;
        m_fpsFrameCount = 0;
    }
}

void Timer::Reset() {
    m_startTime = std::chrono::high_resolution_clock::now();
    m_lastFrameTime = m_startTime;
    m_deltaTime = 0.0f;
    m_totalTime = 0.0f;
    m_frameCount = 0;
    m_fps = 0.0f;
    m_fpsAccumulator = 0.0f;
    m_fpsFrameCount = 0;
}

f64 Timer::GetTimeSinceStart() const {
    auto now = std::chrono::high_resolution_clock::now();
    std::chrono::duration<f64> elapsed = now - m_startTime;
    return elapsed.count();
}

} // namespace nebula
