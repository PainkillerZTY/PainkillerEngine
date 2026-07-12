#pragma once

#include "Types.h"

namespace painkiller {

// ============================================================
// Simplex Noise Generator for terrain generation
// ============================================================
class Noise {
public:
    Noise(i32 seed = 42);

    // 2D noise (for heightmaps)
    f32 Noise2D(f32 x, f32 y) const;

    // 3D noise (for caves, overhangs)
    f32 Noise3D(f32 x, f32 y, f32 z) const;

    // Fractal Brownian Motion - multiple octaves
    f32 FBM2D(f32 x, f32 y, i32 octaves = 4, f32 lacunarity = 2.0f, f32 gain = 0.5f) const;
    f32 FBM3D(f32 x, f32 y, f32 z, i32 octaves = 4, f32 lacunarity = 2.0f, f32 gain = 0.5f) const;

    // Utility: map noise [-1,1] to [0,1]
    static f32 Normalize(f32 value);

private:
    static f32 Grad(i32 hash, f32 x, f32 y, f32 z);
    static f32 Grad2D(i32 hash, f32 x, f32 y);
    static i32 FastFloor(f32 x);

    i32 m_seed;
    std::array<i32, 512> m_perm;
};

} // namespace painkiller
