#pragma once

#include "Types.h"
#include "RenderTypes.h"
#include <string>

namespace nebula {

struct Shader {
    ShaderStage stage;
    std::string source;
    std::string entryPoint;
    u32 handle = 0; // Backend-specific handle
    
    virtual ~Shader() = default;
};

} // namespace nebula
