#include "ResourceManager.h"
#include "Logger.h"
 #include "TextureLoader.h"

namespace painkiller {

ResourceManager::ResourceManager(Renderer* renderer)
    : m_renderer(renderer) {
}

ResourceManager::~ResourceManager() {
    Shutdown();
}

MeshHandle ResourceManager::LoadMesh(const std::string& name, const MeshData& data) {
    auto it = m_meshCache.find(name);
    if (it != m_meshCache.end()) return it->second;
    
    if (!m_renderer) return kInvalidHandle;
    
    MeshHandle handle = m_renderer->CreateMesh(data);
    m_meshCache[name] = handle;
    LOG_INFO("Loaded mesh: {}", name);
    return handle;
}

MeshHandle ResourceManager::GetMesh(const std::string& name) {
    auto it = m_meshCache.find(name);
    return it != m_meshCache.end() ? it->second : kInvalidHandle;
}

bool ResourceManager::HasMesh(const std::string& name) const {
    return m_meshCache.find(name) != m_meshCache.end();
}

ShaderHandle ResourceManager::LoadShader(const std::string& name, ShaderStage stage, const std::string& source) {
    (void)name;
    if (!m_renderer) return kInvalidHandle;
    
    ShaderDesc desc;
    desc.stage = stage;
    desc.source = source;
    return m_renderer->CreateShader(desc);
}

TextureHandle ResourceManager::LoadTexture(const std::string& name, const std::string& filepath) {
    auto it = m_textureCache.find(name);
    if (it != m_textureCache.end()) return it->second;
    
     // Load texture from file using TextureLoader
     TextureDesc desc;
     u8* data = nullptr;
     if (!TextureLoader::LoadFromFile(filepath, desc, data)) {
         return kInvalidHandle;
     }
     
     TextureHandle handle = m_renderer->CreateTexture(desc);
     TextureLoader::FreeLoadedData(data);
     m_textureCache[name] = handle;
     LOG_INFO("Loaded texture: {} -> {}", name, filepath);
     return handle;
}

TextureHandle ResourceManager::CreateTexture(const std::string& name, const TextureDesc& desc) {
    if (!m_renderer) return kInvalidHandle;
    
    TextureHandle handle = m_renderer->CreateTexture(desc);
    m_textureCache[name] = handle;
    return handle;
}

void ResourceManager::Shutdown() {
    if (!m_renderer) return;
    
    for (auto it = m_meshCache.begin(); it != m_meshCache.end(); ++it) {
        m_renderer->DestroyMesh(it->second);
    }
    m_meshCache.clear();
    
    for (auto it = m_textureCache.begin(); it != m_textureCache.end(); ++it) {
        m_renderer->DestroyTexture(it->second);
    }
    m_textureCache.clear();
    
    LOG_INFO("ResourceManager shut down");
}

} // namespace painkiller
