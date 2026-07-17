#include "Renderer.h"
#include "OpenGLRenderer.h"
#include "DirectX11Renderer.h"
#include "VulkanRenderer.h"

namespace painkiller {

Renderer* Renderer::Create(RenderBackend backend) {
    switch (backend) {
        case RenderBackend::OpenGL:
            return OpenGLRenderer::Create();
        case RenderBackend::DirectX11:
            return DirectX11Renderer::Create();
        case RenderBackend::Vulkan:
            return VulkanRenderer::Create();
    }
    return nullptr;
}

} // namespace painkiller
