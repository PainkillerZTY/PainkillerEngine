#pragma once

#include "Types.h"
#include "Renderer.h"
#include <string>
#include <unordered_map>
#include <memory>

namespace painkiller {

// ?? Resource Manager ??
// Handles loading, caching, and lifetime of game resources.
class ResourceManager {
public:
    ResourceManager(Renderer* renderer);
    ~ResourceManager();

    // ?? Meshes ??
    MeshHandle LoadMesh(const std::string& name, const MeshData& data);
    MeshHandle GetMesh(const std::string& name);
    bool HasMesh(const std::string& name) const;

    // ?? Shaders ??
    ShaderHandle LoadShader(const std::string& name, ShaderStage stage, const std::string& source);
    
    // ?? Textures ??
    TextureHandle LoadTexture(const std::string& name, const std::string& filepath);
    TextureHandle CreateTexture(const std::string& name, const TextureDesc& desc);

    // ?? Cleanup ??
    void Shutdown();

private:
    Renderer* m_renderer;
    
    struct MeshEntry {
        MeshHandle handle;
        std::string name;
    };
    
    struct TextureEntry {
        TextureHandle handle;
        std::string name;
    };
    
    std::unordered_map<std::string, MeshHandle>    m_meshCache;
    std::unordered_map<std::string, TextureHandle> m_textureCache;
};

} // namespace painkiller
