#include "Noise.h"
#include <cmath>
#include <algorithm>

namespace nebula {

// Classic permutation table (from Ken Perlin)
static const i32 kPerm[256] = {
    151,160,137, 91, 90, 15,131, 13,201, 95, 96, 53,194,233,  7,225,
    140, 36,103, 30, 69,142,  8, 99, 37,240, 21, 10, 23,190,  6,148,
    247,120,234, 75,  0, 26,197, 62, 94,252,219,203,117, 35, 11, 32,
     57,177, 33, 88,237,149, 56, 87,174, 20,125,136,171,168, 68,175,
     74,165, 71,134,139, 48, 27,166, 77,146,158,231, 83,111,229,122,
     60,211,133,230,220,105, 92, 41, 55, 46,245, 40,244,102,143, 54,
     65, 25, 63,161,  1,216, 80, 73,209, 76,132,187,208, 89, 18,169,
    200,196,135,130,116,188,159, 86,164,100,109,198,173,186,  3, 64,
     52,217,226,250,124,123,  5,202, 38,147,118,126,255, 82, 85,212,
    207,206, 59,227, 47, 16, 58, 17,182,189, 28, 42,223,183,170,213,
    119,248,152,  2, 44,154,163, 70,221,153,101,155,167, 43,172,  9,
    129, 22, 39,253, 19, 98,108,110, 79,113,224,232,178,185,112,104,
    218,246, 97,228,251, 34,242,193,238,210,144, 12,191,179,162,241,
     81, 51,145,235,249, 14,239,107, 49,192,214, 31,181,199,106,157,
    184, 84,204,176,115,121, 50, 45,127,  4,150,254,138,236,205, 93,
    222,114, 67, 29, 24, 72,243,141,128,195, 78, 66,215, 61,156,180,
};

Noise::Noise(i32 seed) : m_seed(seed) {
    // Build extended permutation table with seed-based shuffle
    for (i32 i = 0; i < 256; ++i) {
        m_perm[i] = kPerm[i];
    }
    // Simple shuffle using seed
    std::srand((u32)m_seed);
    for (i32 i = 255; i > 0; --i) {
        i32 j = std::rand() % (i + 1);
        std::swap(m_perm[i], m_perm[j]);
    }
    for (i32 i = 0; i < 256; ++i) {
        m_perm[256 + i] = m_perm[i];
    }
}

i32 Noise::FastFloor(f32 x) {
    return (x > 0) ? (i32)x : (i32)x - 1;
}

f32 Noise::Grad(i32 hash, f32 x, f32 y, f32 z) {
    i32 h = hash & 15;
    f32 u = (h < 8) ? x : y;
    f32 v = (h < 4) ? y : ((h == 12 || h == 14) ? x : z);
    return ((h & 1) == 0 ? u : -u) + ((h & 2) == 0 ? v : -v);
}

f32 Noise::Grad2D(i32 hash, f32 x, f32 y) {
    i32 h = hash & 3;
    f32 u = (h & 1) ? -x : x;
    f32 v = (h & 2) ? -y : y;
    return u + v;
}

f32 Noise::Noise3D(f32 x, f32 y, f32 z) const {
    i32 X = FastFloor(x) & 255;
    i32 Y = FastFloor(y) & 255;
    i32 Z = FastFloor(z) & 255;

    x -= floorf(x);
    y -= floorf(y);
    z -= floorf(z);

    f32 u = x * x * x * (x * (x * 6.0f - 15.0f) + 10.0f);
    f32 v = y * y * y * (y * (y * 6.0f - 15.0f) + 10.0f);
    f32 w = z * z * z * (z * (z * 6.0f - 15.0f) + 10.0f);

    i32 A  = m_perm[X] + Y;
    i32 AA = m_perm[A] + Z;
    i32 AB = m_perm[A + 1] + Z;
    i32 B  = m_perm[X + 1] + Y;
    i32 BA = m_perm[B] + Z;
    i32 BB = m_perm[B + 1] + Z;

    f32 x1 = Lerp(
        Lerp(Lerp(Grad(m_perm[AA], x, y, z),     Grad(m_perm[BA], x-1, y, z),     u),
             Lerp(Grad(m_perm[AB], x, y-1, z),   Grad(m_perm[BB], x-1, y-1, z),   u), v),
        Lerp(Lerp(Grad(m_perm[AA+1], x, y, z-1), Grad(m_perm[BA+1], x-1, y, z-1), u),
             Lerp(Grad(m_perm[AB+1], x, y-1, z-1), Grad(m_perm[BB+1], x-1, y-1, z-1), u), v), w);
    return x1;
}

f32 Noise::Noise2D(f32 x, f32 y) const {
    i32 X = FastFloor(x) & 255;
    i32 Y = FastFloor(y) & 255;

    x -= floorf(x);
    y -= floorf(y);

    f32 u = x * x * x * (x * (x * 6.0f - 15.0f) + 10.0f);
    f32 v = y * y * y * (y * (y * 6.0f - 15.0f) + 10.0f);

    i32 A = m_perm[X] + Y;
    i32 B = m_perm[X + 1] + Y;

    return Lerp(
        Lerp(Grad2D(m_perm[A], x, y),     Grad2D(m_perm[B], x-1, y),     u),
        Lerp(Grad2D(m_perm[A+1], x, y-1), Grad2D(m_perm[B+1], x-1, y-1), u), v);
}

f32 Noise::FBM2D(f32 x, f32 y, i32 octaves, f32 lacunarity, f32 gain) const {
    f32 value = 0.0f;
    f32 amplitude = 1.0f;
    f32 frequency = 1.0f;
    f32 maxVal = 0.0f;

    for (i32 i = 0; i < octaves; ++i) {
        value += amplitude * Noise2D(x * frequency, y * frequency);
        maxVal += amplitude;
        amplitude *= gain;
        frequency *= lacunarity;
    }
    return value / maxVal;
}

f32 Noise::FBM3D(f32 x, f32 y, f32 z, i32 octaves, f32 lacunarity, f32 gain) const {
    f32 value = 0.0f;
    f32 amplitude = 1.0f;
    f32 frequency = 1.0f;
    f32 maxVal = 0.0f;

    for (i32 i = 0; i < octaves; ++i) {
        value += amplitude * Noise3D(x * frequency, y * frequency, z * frequency);
        maxVal += amplitude;
        amplitude *= gain;
        frequency *= lacunarity;
    }
    return value / maxVal;
}

f32 Noise::Normalize(f32 value) {
    return (value + 1.0f) / 2.0f;
}

} // namespace nebula
