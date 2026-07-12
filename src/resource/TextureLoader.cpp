 #include "TextureLoader.h"
 #include "Logger.h"
 #include "stb_image.h"
 
 namespace painkiller {
 
 bool TextureLoader::LoadFromFile(const std::string& filepath, TextureDesc& outDesc, u8*& outData) {
     i32 w, h, channels;
     stbi_set_flip_vertically_on_load(1);
     outData = stbi_load(filepath.c_str(), &w, &h, &channels, 4); // Force RGBA
     if (!outData) {
         LOG_ERROR("Failed to load texture: {} (ch:{})", filepath, channels);
         return false;
     }
     outDesc.type = TextureType::Texture2D;
     outDesc.width = (u32)w;
     outDesc.height = (u32)h;
     outDesc.depth = 1;
     outDesc.mipLevels = 1;
     outDesc.format = Format::R8G8B8A8_UNORM;
     outDesc.initialData = outData;
     LOG_INFO("Loaded texture: {} ({}x{})", filepath, w, h);
     return true;
 }
 
 void TextureLoader::FreeLoadedData(u8* data) {
     if (data) stbi_image_free(data);
 }
 
 } // namespace painkiller
