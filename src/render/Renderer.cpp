#include "Renderer.h"
#include "OpenGLRenderer.h"
#ifdef PAINKILLER_HAS_D3D11
#   include "DirectX11Renderer.h"
#endif
#ifdef PAINKILLER_HAS_VULKAN
#   include "VulkanRenderer.h"
#endif

namespace painkiller {

Renderer* Renderer::Create(RenderBackend backend) {
    switch (backend) {
        case RenderBackend::OpenGL:
            return OpenGLRenderer::Create();
        case RenderBackend::DirectX11:
#ifdef PAINKILLER_HAS_D3D11
            return DirectX11Renderer::Create();
#else
            return nullptr;
#endif
        case RenderBackend::Vulkan:
#ifdef PAINKILLER_HAS_VULKAN
            return VulkanRenderer::Create();
#else
            return nullptr;
#endif
    }
    return nullptr;
}

} // namespace painkiller
