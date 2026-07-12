#pragma once

#include "Component.h"
#include "RenderTypes.h"

namespace painkiller {

// ?? Mesh Component ??
struct MeshComponent {
    MeshHandle meshHandle = kInvalidHandle;
    // Will extend with material assignments later
};

} // namespace painkiller
