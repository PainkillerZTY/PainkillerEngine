#include "SoundManager.h"
#include "Logger.h"

namespace nebula {

SoundManager::SoundManager() {}

SoundManager::~SoundManager() {
    Shutdown();
}

void SoundManager::Initialize() {
    m_initialized = true;
    LOG_INFO("SoundManager initialized (using Beep API)");
}

void SoundManager::Shutdown() {
    m_initialized = false;
}

void SoundManager::PlayBlockBreak() {
    if (!m_initialized) return;
    // Short low-pitched noise - simulate block breaking
    Beep(200, 50);
}

void SoundManager::PlayBlockPlace() {
    if (!m_initialized) return;
    // Short thud sound
    Beep(150, 30);
}

void SoundManager::PlayClick() {
    if (!m_initialized) return;
    // UI click
    Beep(800, 20);
}

} // namespace nebula
