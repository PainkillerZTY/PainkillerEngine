#include "Renderer.h"

namespace painkiller {

// ?? Factory ??
Renderer* Renderer::Create(RenderBackend backend) {
    switch (backend) {
        case RenderBackend::OpenGL:
            // OpenGLRenderer defined in OpenGLRenderer.h
            break;
        case RenderBackend::DirectX11:
        case RenderBackend::Vulkan:
            // Not yet implemented
            break;
    }
    return nullptr; // Concrete implementation creates via static method
}

} // namespace painkiller
