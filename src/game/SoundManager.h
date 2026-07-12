#pragma once

#include "Types.h"
#include <string>
#include <windows.h>

namespace painkiller {

// ============================================================
// SoundManager: Simple sound effects via Win32 PlaySound
// ============================================================
class SoundManager {
public:
    SoundManager();
    ~SoundManager();

    void Initialize();
    void Shutdown();

    // Play a simple beep tone (no external WAV files needed)
    void PlayBlockBreak();
    void PlayBlockPlace();
    void PlayClick();

private:
    bool m_initialized = false;
};

} // namespace painkiller
