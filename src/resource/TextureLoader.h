#pragma once

#include "Types.h"
#include "RenderTypes.h"
#include "Renderer.h"
#include <string>

namespace painkiller {

class TextureLoader {
public:
    static bool LoadFromFile(const std::string& filepath, TextureDesc& outDesc, u8*& outData);
    static void FreeLoadedData(u8* data);
};

} // namespace painkiller
