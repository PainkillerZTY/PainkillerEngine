#pragma once

#include "Types.h"
#include "RenderTypes.h"
#include "Renderer.h"
#include <string>

namespace nebula {

class ModelLoader {
public:
    // Placeholder for future OBJ/glTF loader
    static MeshData LoadOBJ(const std::string& filepath);
    static MeshData LoadGLTF(const std::string& filepath);
};

} // namespace nebula
