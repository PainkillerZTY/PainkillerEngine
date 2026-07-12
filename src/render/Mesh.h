#pragma once

#include "Types.h"
#include "RenderTypes.h"
#include "Renderer.h"

namespace painkiller {

struct Mesh {
    MeshData data;
    MeshHandle handle = kInvalidHandle;
    
    virtual ~Mesh() = default;
};

} // namespace painkiller
