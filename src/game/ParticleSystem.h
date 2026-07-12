#pragma once

#include "Types.h"
#include "Vector.h"
#include "Renderer.h"
#include "RenderTypes.h"
#include <vector>

namespace painkiller {

// ============================================================
// Particle: Single particle for block break effects
// ============================================================
struct Particle {
    Vec3 position;
    Vec3 velocity;
    Vec3 color;
    f32 life;       // Remaining life (seconds)
    f32 maxLife;    // Initial life
    f32 size;
};

// ============================================================
// ParticleSystem: Manages block break particle effects
// ============================================================
class ParticleSystem {
public:
    ParticleSystem();
    ~ParticleSystem();

    void Initialize(Renderer* renderer);
    void Shutdown();

    // Spawn particles at a position with block color
    void SpawnBreakParticles(const Vec3& worldPos, const Vec3& blockColor, i32 count = 8);

    // Update all particles
    void Update(f32 deltaTime);

    // Render all particles
    void Render(Renderer* renderer);

    // Access particles for rendering
    const std::vector<Particle>& GetParticles() const { return m_particles; }

private:
    std::vector<Particle> m_particles;
    MeshHandle m_cubeMesh = kInvalidHandle;
    ShaderHandle m_vs = kInvalidHandle;
    ShaderHandle m_fs = kInvalidHandle;
    PipelineHandle m_pipeline = kInvalidHandle;
    bool m_initialized = false;
};

} // namespace painkiller
