#include "ModelLoader.h"
#include "Logger.h"

namespace painkiller {

MeshData ModelLoader::LoadOBJ(const std::string& filepath) {
    LOG_WARN("OBJ loading not yet implemented: {}", filepath);
    return MeshData{};
}

MeshData ModelLoader::LoadGLTF(const std::string& filepath) {
    LOG_WARN("glTF loading not yet implemented: {}", filepath);
    return MeshData{};
}

} // namespace painkiller
