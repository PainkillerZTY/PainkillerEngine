#pragma once

#include "Types.h"
#include "RenderTypes.h"

namespace painkiller {

struct Buffer {
    BufferType type;
    ResourceUsage usage;
    u32 size;
    u32 stride;
    u32 handle = 0; // Backend-specific handle (e.g. VBO, IBO, UBO)
    
    virtual ~Buffer() = default;
};

} // namespace painkiller
