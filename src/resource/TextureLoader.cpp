#include "TextureLoader.h"
#include "Logger.h"

namespace nebula {

bool TextureLoader::LoadFromFile(const std::string& filepath, TextureDesc& outDesc, u8*& outData) {
    LOG_WARN("Texture loading not yet implemented: {}", filepath);
    return false;
}

void TextureLoader::FreeLoadedData(u8* data) {
    // TODO: free via stb_image
    if (data) free(data);
}

} // namespace nebula
