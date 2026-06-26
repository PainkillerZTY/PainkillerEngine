#pragma once

#include "Types.h"
#include "RenderTypes.h"

namespace nebula {

struct Texture {
    u32 width = 0;
    u32 height = 0;
    u32 depth = 1;
    u32 mipLevels = 1;
    Format format = Format::R8G8B8A8_UNORM;
    u32 handle = 0; // Backend-specific handle
    
    virtual ~Texture() = default;
};

} // namespace nebula
