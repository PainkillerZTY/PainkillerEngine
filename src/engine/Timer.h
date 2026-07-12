#pragma once

#include "Types.h"
#include <chrono>

namespace painkiller {

// ?? Timer: High-precision frame timing ??
class Timer {
public:
    Timer();

    void Tick();                     // Call once per frame
    void Reset();
    
    f32 GetDeltaTime() const { return m_deltaTime; }        // Seconds since last frame
    f32 GetTotalTime() const { return m_totalTime; }        // Seconds since start
    f64 GetTimeSinceStart() const;                           // High-precision seconds
    u64 GetFrameCount() const { return m_frameCount; }
    
    // FPS calculation
    f32 GetFPS() const { return m_fps; }
    
private:
    std::chrono::high_resolution_clock::time_point m_startTime;
    std::chrono::high_resolution_clock::time_point m_lastFrameTime;
    
    f32 m_deltaTime = 0.0f;
    f32 m_totalTime = 0.0f;
    u64 m_frameCount = 0;
    
    // FPS smoothing
    f32 m_fps = 0.0f;
    f32 m_fpsAccumulator = 0.0f;
    u32 m_fpsFrameCount = 0;
    static constexpr u32 kFPSUpdateInterval = 30; // Update FPS every N frames
};

} // namespace painkiller
