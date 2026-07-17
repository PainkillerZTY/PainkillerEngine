// ============================================================
// VulkanRenderer.h — Vulkan backend
//
// Implements Renderer interface via Vulkan 1.1+.
// Runtime GLSL→SPIR-V via shaderc, reflection via SPIRV-Reflect.
// Per-pipeline UBO for named uniforms, descriptor set per frame.
// ============================================================
#pragma once

#include "Renderer.h"
#include "RenderTypes.h"
#include "Vector.h"
#include "Matrix.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <vulkan/vulkan.h>
#include <volk.h>

#include <string>
#include <unordered_map>
#include <vector>
#include <array>
#include <optional>

namespace painkiller {

// ------------------------------------------------------------------
// Forward-declare reflection types (we include headers in .cpp)
// ------------------------------------------------------------------
struct VkReflectionUniform {
    std::string name;
    u32         offset = 0;
    u32         size   = 0;
};

// ------------------------------------------------------------------
// Vulkan resource wrappers
// ------------------------------------------------------------------
struct VkShader {
    ShaderStage   stage;
    ShaderHandle  handle = kInvalidHandle;
    VkShaderModule module = VK_NULL_HANDLE;
    std::vector<u32> spirv; // retained for potential re-use
};

struct VkBuffer {
    BufferType     type;
    u32            size   = 0;
    u32            stride = 0;
    VkBuffer       buffer = VK_NULL_HANDLE;
    VkDeviceMemory memory = VK_NULL_HANDLE;
};

struct VkTexture {
    u32              width  = 0;
    u32              height = 0;
    Format           format = Format::R8G8B8A8_UNORM;
    VkImage          image      = VK_NULL_HANDLE;
    VkDeviceMemory   memory     = VK_NULL_HANDLE;
    VkImageView      imageView  = VK_NULL_HANDLE;
    VkSampler        sampler    = VK_NULL_HANDLE;
};

struct VkPipeline {
    // Shader modules (owned here to manage lifetime)
    VkShaderModule vsModule = VK_NULL_HANDLE;
    VkShaderModule psModule = VK_NULL_HANDLE;

    // Pipeline objects
    VkPipeline       pipeline       = VK_NULL_HANDLE;
    VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
    VkDescriptorSetLayout descSetLayout = VK_NULL_HANDLE;
    VkDescriptorSet      descSet        = VK_NULL_HANDLE;

    // Uniform buffer per pipeline (named-uniform storage)
    VkBuffer       uniformBuffer   = VK_NULL_HANDLE;
    VkDeviceMemory uniformMemory   = VK_NULL_HANDLE;
    u32            uniformSize     = 0;

    // Reflection data
    std::vector<VkReflectionUniform> vsSlots;
    std::vector<VkReflectionUniform> psSlots;

    // Shadow copy
    std::vector<u8> shadow;
    bool dirty = false;

    PrimitiveTopology topology = PrimitiveTopology::TriangleList;
};

struct VkMesh {
    VkBuffer       vertexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory vertexMemory = VK_NULL_HANDLE;
    VkBuffer       indexBuffer  = VK_NULL_HANDLE;
    VkDeviceMemory indexMemory  = VK_NULL_HANDLE;
    u32 vertexCount  = 0;
    u32 indexCount   = 0;
    u32 vertexStride = 0;
    bool indexed = false;
};

// ------------------------------------------------------------------
// VulkanRenderer
// ------------------------------------------------------------------
class VulkanRenderer : public Renderer {
public:
    VulkanRenderer()  = default;
    ~VulkanRenderer() override { Shutdown(); }

    bool Initialize(u32 width, u32 height, void* windowHandle) override;
    void Shutdown() override;
    void BeginFrame() override;
    void EndFrame() override;
    void Present() override;
    void ClearColor(f32 r, f32 g, f32 b, f32 a) override;
    void ClearDepth(f32 value = 1.0f) override;
    void SetViewport(const ViewportDesc& desc) override;

    ShaderHandle   CreateShader(const ShaderDesc& desc) override;
    BufferHandle   CreateBuffer(const BufferDesc& desc) override;
    TextureHandle  CreateTexture(const TextureDesc& desc) override;
    PipelineHandle CreatePipelineState(const PipelineStateDesc& desc) override;
    MeshHandle     CreateMesh(const MeshData& data) override;

    void DestroyShader(ShaderHandle handle) override;
    void DestroyBuffer(BufferHandle handle) override;
    void DestroyTexture(TextureHandle handle) override;
    void DestroyPipelineState(PipelineHandle handle) override;
    void DestroyMesh(MeshHandle handle) override;

    void BindPipelineState(PipelineHandle handle) override;
    void BindVertexBuffer(BufferHandle handle, u32 slot) override;
    void BindIndexBuffer(BufferHandle handle) override;
    void BindTexture(TextureHandle handle, u32 slot) override;
    void BindMesh(MeshHandle handle) override;

    void SetUniformBuffer(BufferHandle handle, u32 slot) override;
    void SetUniformMat4(const char* name, const Mat4& matrix) override;
    void SetUniformVec3(const char* name, const Vec3& value) override;
    void SetUniformVec4(const char* name, const Vec4& value) override;
    void SetUniformFloat(const char* name, f32 value) override;
    void SetUniformInt(const char* name, i32 value) override;

    void Draw(u32 vertexCount, u32 startVertex = 0) override;
    void DrawIndexed(u32 indexCount, u32 startIndex = 0, i32 baseVertex = 0) override;
    void DrawMesh(MeshHandle mesh) override;
    void OnResize(u32 width, u32 height) override;

    RenderBackend GetBackendType() const override { return RenderBackend::Vulkan; }
    const char*   GetBackendName() const override { return "Vulkan 1.1"; }

    static VulkanRenderer* Create() { return new VulkanRenderer(); }

private:
    // --- Vulkan initialization sub-steps ---
    bool CreateInstance();
    bool SelectPhysicalDevice();
    bool CreateLogicalDevice();
    bool CreateSurface();
    bool CreateSwapChain(u32 width, u32 height);
    bool CreateRenderPass();
    bool CreateCommandPool();
    bool CreateSyncObjects();
    bool CreateDescriptorPool();
    bool CreateFramebuffers();
    void DestroySwapChain();

    // --- Pipeline helpers ---
    VkShaderModule CompileToSPIRV(ShaderStage stage, const std::string& source,
                                  std::vector<u32>& outSPIRV);
    void ReflectUniforms(const std::vector<u32>& spirv,
                         std::vector<VkReflectionUniform>& outSlots,
                         u32& outTotalSize);
    VkFormat ToVkFormat(Format fmt) const;

    // --- Resource helpers ---
    u32 FindMemoryType(u32 typeFilter, VkMemoryPropertyFlags properties) const;
    void CreateVkBuffer(VkDeviceSize size, VkBufferUsageFlags usage,
                        VkMemoryPropertyFlags memoryProps,
                        VkBuffer& buffer, VkDeviceMemory& memory,
                        const void* data = nullptr) const;
    VkCommandBuffer BeginSingleTimeCommands() const;
    void EndSingleTimeCommands(VkCommandBuffer cmd) const;
    void TransitionImageLayout(VkImage image, VkFormat format,
                               VkImageLayout oldLayout, VkImageLayout newLayout) const;
    void CopyBufferToImage(VkBuffer buffer, VkImage image,
                           u32 width, u32 height) const;

    // --- Flush ---
    void FlushUniforms();

    // --- Vulkan handles ---
    VkInstance                m_instance         = VK_NULL_HANDLE;
    VkPhysicalDevice          m_physicalDevice   = VK_NULL_HANDLE;
    VkDevice                  m_device           = VK_NULL_HANDLE;
    VkQueue                   m_graphicsQueue    = VK_NULL_HANDLE;
    VkQueue                   m_presentQueue     = VK_NULL_HANDLE;
    VkSurfaceKHR              m_surface          = VK_NULL_HANDLE;

    // --- Swap chain ---
    VkSwapchainKHR            m_swapChain        = VK_NULL_HANDLE;
    std::vector<VkImage>      m_swapChainImages;
    std::vector<VkImageView>  m_swapChainImageViews;
    std::vector<VkFramebuffer> m_swapChainFramebuffers;
    VkFormat                  m_swapChainFormat  = VK_FORMAT_B8G8R8A8_UNORM;
    VkExtent2D                m_swapChainExtent  = {};
    VkRenderPass              m_renderPass       = VK_NULL_HANDLE;

    // --- Depth resources ---
    VkImage                   m_depthImage       = VK_NULL_HANDLE;
    VkDeviceMemory            m_depthImageMemory = VK_NULL_HANDLE;
    VkImageView               m_depthImageView   = VK_NULL_HANDLE;

    // --- Command ---
    VkCommandPool             m_commandPool      = VK_NULL_HANDLE;
    VkCommandBuffer           m_commandBuffer    = VK_NULL_HANDLE;

    // --- Sync ---
    VkSemaphore               m_imageAvailableSemaphore = VK_NULL_HANDLE;
    VkSemaphore               m_renderFinishedSemaphore = VK_NULL_HANDLE;
    VkFence                   m_inFlightFence           = VK_NULL_HANDLE;
    bool                      m_fenceWaiting            = false;

    // --- Descriptor ---
    VkDescriptorPool          m_descriptorPool   = VK_NULL_HANDLE;

    // --- Queue family indices ---
    u32 m_graphicsFamily = 0;
    u32 m_presentFamily  = 0;

    // --- Resource arrays ---
    std::vector<VkShader>    m_shaders;
    std::vector<painkiller::VkBuffer>   m_buffers;
    std::vector<VkTexture>   m_textures;
    std::vector<painkiller::VkPipeline> m_pipelines;
    std::vector<VkMesh>      m_meshes;

    // --- Current pipeline ---
    painkiller::VkPipeline* m_activePipeline = nullptr;
    VkPipelineLayout m_activePipelineLayout = VK_NULL_HANDLE;

    // --- Default pipeline objects ---
    VkPipelineLayout m_defaultPipelineLayout = VK_NULL_HANDLE;

    // --- State ---
    u32  m_width  = 0;
    u32  m_height = 0;
    HWND m_hwnd   = nullptr;
    bool m_initialized = false;
    u32  m_currentFrame = 0;
};

} // namespace painkiller
