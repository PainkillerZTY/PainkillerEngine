// ============================================================
// VulkanRenderer.cpp — Vulkan backend implementation
//
// Full Vulkan 1.1 implementation of the Renderer interface.
// Dependencies: Vulkan SDK (volk, shaderc, spirv-reflect)
// ============================================================
#include "VulkanRenderer.h"
#include "Logger.h"

#define VOLK_IMPLEMENTATION
#include <volk.h>

#include <shaderc/shaderc.h>
#include <spirv_reflect.h>

#include <algorithm>
#include <cstring>
#include <set>
#include <limits>

// Vulkan function loading — volk does it globally
#pragma comment(lib, "vulkan-1.lib")

namespace painkiller {

// ==================================================================
// Constants
// ==================================================================
static constexpr u32 MAX_UBO_SIZE = 4096;

// ==================================================================
// Format conversion
// ==================================================================
VkFormat VulkanRenderer::ToVkFormat(Format fmt) const {
    switch (fmt) {
        case Format::R8G8B8A8_UNORM:         return VK_FORMAT_R8G8B8A8_UNORM;
        case Format::R8G8B8A8_SRGB:          return VK_FORMAT_R8G8B8A8_SRGB;
        case Format::R32G32B32A32_FLOAT:     return VK_FORMAT_R32G32B32A32_FLOAT;
        case Format::R32G32B32_FLOAT:        return VK_FORMAT_R32G32B32_FLOAT;
        case Format::R32G32_FLOAT:           return VK_FORMAT_R32G32_FLOAT;
        case Format::R32_FLOAT:              return VK_FORMAT_R32_FLOAT;
        case Format::D32_FLOAT:              return VK_FORMAT_D32_SFLOAT;
        case Format::D24_UNORM_S8_UINT:      return VK_FORMAT_D24_UNORM_S8_UINT;
        default: return VK_FORMAT_UNDEFINED;
    }
}

static VkPrimitiveTopology ToVkTopology(PrimitiveTopology t) {
    switch (t) {
        case PrimitiveTopology::TriangleList:  return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        case PrimitiveTopology::TriangleStrip: return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
        case PrimitiveTopology::LineList:      return VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
        case PrimitiveTopology::LineStrip:     return VK_PRIMITIVE_TOPOLOGY_LINE_STRIP;
        case PrimitiveTopology::PointList:     return VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
        default: return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    }
}

static VkCullModeFlags ToVkCullMode(CullMode m) {
    switch (m) {
        case CullMode::None:  return VK_CULL_MODE_NONE;
        case CullMode::Front: return VK_CULL_MODE_FRONT_BIT;
        case CullMode::Back:  return VK_CULL_MODE_BACK_BIT;
        default: return VK_CULL_MODE_BACK_BIT;
    }
}

static VkPolygonMode ToVkFillMode(FillMode m) {
    return m == FillMode::Wireframe ? VK_POLYGON_MODE_LINE : VK_POLYGON_MODE_FILL;
}

static VkCompareOp ToVkCompare(CompareFunction f) {
    switch (f) {
        case CompareFunction::Never:         return VK_COMPARE_OP_NEVER;
        case CompareFunction::Less:          return VK_COMPARE_OP_LESS;
        case CompareFunction::Equal:         return VK_COMPARE_OP_EQUAL;
        case CompareFunction::LessEqual:     return VK_COMPARE_OP_LESS_OR_EQUAL;
        case CompareFunction::Greater:       return VK_COMPARE_OP_GREATER;
        case CompareFunction::NotEqual:      return VK_COMPARE_OP_NOT_EQUAL;
        case CompareFunction::GreaterEqual:  return VK_COMPARE_OP_GREATER_OR_EQUAL;
        case CompareFunction::Always:        return VK_COMPARE_OP_ALWAYS;
        default: return VK_COMPARE_OP_LESS;
    }
}

static VkBlendFactor ToVkSrcBlend(BlendMode m) {
    switch (m) {
        case BlendMode::Opaque:      return VK_BLEND_FACTOR_ONE;
        case BlendMode::AlphaBlend:  return VK_BLEND_FACTOR_SRC_ALPHA;
        case BlendMode::Additive:    return VK_BLEND_FACTOR_ONE;
        case BlendMode::Multiply:    return VK_BLEND_FACTOR_ZERO;
        default: return VK_BLEND_FACTOR_ONE;
    }
}

static VkBlendFactor ToVkDstBlend(BlendMode m) {
    switch (m) {
        case BlendMode::Opaque:      return VK_BLEND_FACTOR_ZERO;
        case BlendMode::AlphaBlend:  return VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        case BlendMode::Additive:    return VK_BLEND_FACTOR_ONE;
        case BlendMode::Multiply:    return VK_BLEND_FACTOR_SRC_COLOR;
        default: return VK_BLEND_FACTOR_ZERO;
    }
}

// ==================================================================
// Helper: create VkBuffer with optional initial data
// ==================================================================
void VulkanRenderer::CreateVkBuffer(VkDeviceSize size, VkBufferUsageFlags usage,
                                    VkMemoryPropertyFlags memoryProps,
                                    VkBuffer& buffer, VkDeviceMemory& memory,
                                    const void* data) const
{
    VkBufferCreateInfo ci = {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    ci.size  = size;
    ci.usage = usage;
    ci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    vkCreateBuffer(m_device, &ci, nullptr, &buffer);

    VkMemoryRequirements memReq;
    vkGetBufferMemoryRequirements(m_device, buffer, &memReq);

    u32 typeIndex = FindMemoryType(memReq.memoryTypeBits, memoryProps);

    VkMemoryAllocateInfo ai = {VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
    ai.allocationSize  = memReq.size;
    ai.memoryTypeIndex = typeIndex;
    vkAllocateMemory(m_device, &ai, nullptr, &memory);
    vkBindBufferMemory(m_device, buffer, memory, 0);

    if (data) {
        void* mapped;
        vkMapMemory(m_device, memory, 0, size, 0, &mapped);
        memcpy(mapped, data, (size_t)size);
        vkUnmapMemory(m_device, memory);
    }
}

u32 VulkanRenderer::FindMemoryType(u32 typeFilter, VkMemoryPropertyFlags properties) const {
    VkPhysicalDeviceMemoryProperties memProps;
    vkGetPhysicalDeviceMemoryProperties(m_physicalDevice, &memProps);

    for (u32 i = 0; i < memProps.memoryTypeCount; ++i) {
        if ((typeFilter & (1u << i)) &&
            (memProps.memoryTypes[i].propertyFlags & properties) == properties)
            return i;
    }
    LOG_ERROR("Failed to find suitable memory type");
    return 0;
}

// ==================================================================
// Single-time command buffer helpers
// ==================================================================
VkCommandBuffer VulkanRenderer::BeginSingleTimeCommands() const {
    VkCommandBufferAllocateInfo ai = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
    ai.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    ai.commandPool        = m_commandPool;
    ai.commandBufferCount = 1;

    VkCommandBuffer cmd;
    vkAllocateCommandBuffers(m_device, &ai, &cmd);

    VkCommandBufferBeginInfo bi = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    bi.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(cmd, &bi);
    return cmd;
}

void VulkanRenderer::EndSingleTimeCommands(VkCommandBuffer cmd) const {
    vkEndCommandBuffer(cmd);

    VkSubmitInfo si = {VK_STRUCTURE_TYPE_SUBMIT_INFO};
    si.commandBufferCount = 1;
    si.pCommandBuffers    = &cmd;
    vkQueueSubmit(m_graphicsQueue, 1, &si, VK_NULL_HANDLE);
    vkQueueWaitIdle(m_graphicsQueue);

    vkFreeCommandBuffers(m_device, m_commandPool, 1, &cmd);
}

void VulkanRenderer::TransitionImageLayout(VkImage image, VkFormat /*format*/,
                                           VkImageLayout oldLayout, VkImageLayout newLayout) const
{
    VkCommandBuffer cmd = BeginSingleTimeCommands();

    VkImageMemoryBarrier barrier = {VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
    barrier.oldLayout        = oldLayout;
    barrier.newLayout        = newLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image            = image;
    barrier.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};

    VkPipelineStageFlags srcStage, dstStage;
    if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    } else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    } else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT |
                                VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        dstStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    } else {
        // Default: undefined → shader read
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    }

    vkCmdPipelineBarrier(cmd, srcStage, dstStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);
    EndSingleTimeCommands(cmd);
}

void VulkanRenderer::CopyBufferToImage(VkBuffer buffer, VkImage image,
                                       u32 width, u32 height) const
{
    VkCommandBuffer cmd = BeginSingleTimeCommands();

    VkBufferImageCopy region = {};
    region.bufferOffset      = 0;
    region.bufferRowLength   = 0;
    region.bufferImageHeight = 0;
    region.imageSubresource  = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
    region.imageOffset       = {0, 0, 0};
    region.imageExtent       = {width, height, 1};

    vkCmdCopyBufferToImage(cmd, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
    EndSingleTimeCommands(cmd);
}

// ==================================================================
// Shader compilation via shaderc
// ==================================================================
VkShaderModule VulkanRenderer::CompileToSPIRV(ShaderStage stage,
                                              const std::string& source,
                                              std::vector<u32>& outSPIRV)
{
    shaderc_compiler_t compiler = shaderc_compiler_initialize();
    if (!compiler) {
        LOG_ERROR("shaderc_compiler_initialize failed");
        return VK_NULL_HANDLE;
    }

    shaderc_shader_kind kind = (stage == ShaderStage::Vertex)
        ? shaderc_glsl_vertex_shader
        : shaderc_glsl_fragment_shader;

    shaderc_compilation_result_t result = shaderc_compile_into_spv(
        compiler, source.c_str(), source.size(), kind, "shader", "main", nullptr);

    bool ok = shaderc_result_get_compilation_status(result) == shaderc_compilation_status_success;
    if (!ok) {
        LOG_ERROR("Shader compile error:\n{}", shaderc_result_get_error_message(result));
        shaderc_result_release(result);
        shaderc_compiler_release(compiler);
        return VK_NULL_HANDLE;
    }

    size_t wordCount = shaderc_result_get_length(result) / sizeof(u32);
    const u32* words = (const u32*)shaderc_result_get_bytes(result);
    outSPIRV.assign(words, words + wordCount);

    VkShaderModuleCreateInfo smci = {VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
    smci.codeSize = shaderc_result_get_length(result);
    smci.pCode    = words;

    VkShaderModule module;
    VkResult vr = vkCreateShaderModule(m_device, &smci, nullptr, &module);
    shaderc_result_release(result);
    shaderc_compiler_release(compiler);

    if (vr != VK_SUCCESS) {
        LOG_ERROR("vkCreateShaderModule failed: {}", int(vr));
        return VK_NULL_HANDLE;
    }
    return module;
}

// ==================================================================
// Uniform reflection via SPIRV-Reflect
// ==================================================================
void VulkanRenderer::ReflectUniforms(const std::vector<u32>& spirv,
                                     std::vector<VkReflectionUniform>& outSlots,
                                     u32& outTotalSize)
{
    outSlots.clear();
    outTotalSize = 0;

    spv_reflect::ShaderModule reflection(spirv.size() * sizeof(u32), spirv.data());
    if (reflection.GetResult() != SPV_REFLECT_RESULT_SUCCESS) return;

    u32 bindingCount = 0;
    SpvReflectDescriptorBinding* bindings[16] = {};
    reflection.EnumerateDescriptorBindings(&bindingCount, nullptr);
    if (bindingCount > 0) {
        reflection.EnumerateDescriptorBindings(&bindingCount, bindings);
    }

    u32 varCount = 0;
    SpvReflectBlockVariable* vars[256] = {};
    if (bindingCount > 0 && bindings[0]) {
        varCount = bindings[0]->block.member_count;
        for (u32 i = 0; i < varCount && i < 256; ++i) {
            vars[i] = &bindings[0]->block.members[i];
        }
    }

    outTotalSize = (bindingCount > 0 && bindings[0])
        ? bindings[0]->block.size : 0;

    for (u32 i = 0; i < varCount && i < 256; ++i) {
        outSlots.push_back({ vars[i]->name, vars[i]->offset, vars[i]->size });
    }
}

// ==================================================================
// Initialize
// ==================================================================
bool VulkanRenderer::Initialize(u32 width, u32 height, void* windowHandle) {
    LOG_INFO("Initializing Vulkan...");
    m_width  = width;
    m_height = height;
    m_hwnd   = (HWND)windowHandle;

    volkInitialize();

    if (!CreateInstance())        return false;
    if (!SelectPhysicalDevice())  return false;
    if (!CreateSurface())         return false;
    if (!CreateLogicalDevice())   return false;
    if (!CreateSwapChain(width, height)) return false;
    if (!CreateRenderPass())      return false;
    if (!CreateDescriptorPool())  return false;
    if (!CreateCommandPool())     return false;
    if (!CreateSyncObjects())     return false;
    if (!CreateFramebuffers())    return false;

    // Allocate primary command buffer
    VkCommandBufferAllocateInfo ai = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
    ai.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    ai.commandPool        = m_commandPool;
    ai.commandBufferCount = 1;
    vkAllocateCommandBuffers(m_device, &ai, &m_commandBuffer);

    m_initialized = true;
    LOG_INFO("Vulkan initialized successfully");
    return true;
}

// ==================================================================
// Instance
// ==================================================================
bool VulkanRenderer::CreateInstance() {
    volkLoadInstance(nullptr); // Load global functions

    // Check validation layer support
    const char* layers[] = { "VK_LAYER_KHRONOS_validation" };
    u32 layerCount = 0;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
    std::vector<VkLayerProperties> availLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availLayers.data());

    bool validate = false;
    for (const auto& l : availLayers) {
        if (strcmp(l.layerName, "VK_LAYER_KHRONOS_validation") == 0) {
            validate = true;
            break;
        }
    }

    const char* extensions[] = {
        VK_KHR_SURFACE_EXTENSION_NAME,
        VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
#ifdef _DEBUG
        VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
#endif
    };

    VkApplicationInfo appInfo = {VK_STRUCTURE_TYPE_APPLICATION_INFO};
    appInfo.pApplicationName = "PainkillerEngine";
    appInfo.applicationVersion = VK_MAKE_VERSION(0, 1, 0);
    appInfo.pEngineName = "PainkillerEngine";
    appInfo.engineVersion = VK_MAKE_VERSION(0, 1, 0);
    appInfo.apiVersion = VK_API_VERSION_1_1;

    VkInstanceCreateInfo ci = {VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
    ci.pApplicationInfo        = &appInfo;
    ci.enabledExtensionCount   = (u32)(sizeof(extensions) / sizeof(extensions[0]));
    ci.ppEnabledExtensionNames = extensions;

    if (validate) {
        ci.enabledLayerCount   = 1;
        ci.ppEnabledLayerNames = layers;
    }

    if (vkCreateInstance(&ci, nullptr, &m_instance) != VK_SUCCESS) {
        LOG_ERROR("vkCreateInstance failed");
        return false;
    }

    volkLoadInstance(m_instance);
    LOG_INFO("Vulkan instance created{}", validate ? " (validation layers)" : "");
    return true;
}

// ==================================================================
// Physical device selection
// ==================================================================
bool VulkanRenderer::SelectPhysicalDevice() {
    u32 count = 0;
    vkEnumeratePhysicalDevices(m_instance, &count, nullptr);
    if (count == 0) { LOG_ERROR("No Vulkan-capable GPU found"); return false; }

    std::vector<VkPhysicalDevice> devices(count);
    vkEnumeratePhysicalDevices(m_instance, &count, devices.data());

    // Prefer discrete GPU
    int bestScore = -1;
    for (const auto& dev : devices) {
        VkPhysicalDeviceProperties props;
        vkGetPhysicalDeviceProperties(dev, &props);
        int score = (props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) ? 1000 : 1;
        if (score > bestScore) {
            bestScore = score;
            m_physicalDevice = dev;
        }
    }

    if (m_physicalDevice == VK_NULL_HANDLE) {
        LOG_ERROR("No suitable GPU found");
        return false;
    }

    VkPhysicalDeviceProperties props;
    vkGetPhysicalDeviceProperties(m_physicalDevice, &props);
    LOG_INFO("GPU: {} (Vulkan {}.{}.{})", props.deviceName,
             VK_VERSION_MAJOR(props.apiVersion),
             VK_VERSION_MINOR(props.apiVersion),
             VK_VERSION_PATCH(props.apiVersion));
    return true;
}

// ==================================================================
// Logical device
// ==================================================================
bool VulkanRenderer::CreateLogicalDevice() {
    // Find queue families
    u32 count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(m_physicalDevice, &count, nullptr);
    std::vector<VkQueueFamilyProperties> families(count);
    vkGetPhysicalDeviceQueueFamilyProperties(m_physicalDevice, &count, families.data());

    int graphicsIdx = -1, presentIdx = -1;
    for (u32 i = 0; i < count; ++i) {
        if (families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            graphicsIdx = (int)i;
            // Check present support
            VkBool32 presentSupport = VK_FALSE;
            vkGetPhysicalDeviceSurfaceSupportKHR(m_physicalDevice, i, m_surface, &presentSupport);
            if (presentSupport) presentIdx = (int)i;
        }
    }

    if (graphicsIdx < 0) { LOG_ERROR("No graphics queue family"); return false; }
    if (presentIdx < 0)  { LOG_ERROR("No present queue family");  return false; }

    m_graphicsFamily = (u32)graphicsIdx;
    m_presentFamily  = (u32)presentIdx;

    std::set<u32> uniqueFamilies = { m_graphicsFamily, m_presentFamily };
    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    f32 queuePriority = 1.0f;
    for (u32 fam : uniqueFamilies) {
        VkDeviceQueueCreateInfo qci = {VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};
        qci.queueFamilyIndex = fam;
        qci.queueCount       = 1;
        qci.pQueuePriorities = &queuePriority;
        queueCreateInfos.push_back(qci);
    }

    const char* devExtensions[] = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
    };

    VkPhysicalDeviceFeatures features = {};

    VkDeviceCreateInfo ci = {VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};
    ci.queueCreateInfoCount    = (u32)queueCreateInfos.size();
    ci.pQueueCreateInfos      = queueCreateInfos.data();
    ci.enabledExtensionCount  = 1;
    ci.ppEnabledExtensionNames = devExtensions;
    ci.pEnabledFeatures       = &features;

    if (vkCreateDevice(m_physicalDevice, &ci, nullptr, &m_device) != VK_SUCCESS) {
        LOG_ERROR("vkCreateDevice failed");
        return false;
    }

    volkLoadDevice(m_device);
    vkGetDeviceQueue(m_device, m_graphicsFamily, 0, &m_graphicsQueue);
    vkGetDeviceQueue(m_device, m_presentFamily, 0, &m_presentQueue);
    LOG_INFO("Logical device created (graphics={}, present={})", m_graphicsFamily, m_presentFamily);
    return true;
}

// ==================================================================
// Surface
// ==================================================================
bool VulkanRenderer::CreateSurface() {
    VkWin32SurfaceCreateInfoKHR ci = {VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR};
    ci.hwnd   = m_hwnd;
    ci.hinstance = (HINSTANCE)GetWindowLongPtr(m_hwnd, GWLP_HINSTANCE);

    if (vkCreateWin32SurfaceKHR(m_instance, &ci, nullptr, &m_surface) != VK_SUCCESS) {
        LOG_ERROR("vkCreateWin32SurfaceKHR failed");
        return false;
    }
    return true;
}

// ==================================================================
// Swap chain
// ==================================================================
bool VulkanRenderer::CreateSwapChain(u32 width, u32 height) {
    // Query surface capabilities
    VkSurfaceCapabilitiesKHR caps;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_physicalDevice, m_surface, &caps);

    u32 formatCount = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(m_physicalDevice, m_surface, &formatCount, nullptr);
    std::vector<VkSurfaceFormatKHR> formats(formatCount);
    vkGetPhysicalDeviceSurfaceFormatsKHR(m_physicalDevice, m_surface, &formatCount, formats.data());

    VkSurfaceFormatKHR surfaceFormat = formats[0];
    for (const auto& f : formats) {
        if (f.format == VK_FORMAT_B8G8R8A8_UNORM && f.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            surfaceFormat = f;
            break;
        }
    }
    m_swapChainFormat = surfaceFormat.format;

    // Present mode
    u32 presentModeCount = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(m_physicalDevice, m_surface, &presentModeCount, nullptr);
    std::vector<VkPresentModeKHR> presentModes(presentModeCount);
    vkGetPhysicalDeviceSurfacePresentModesKHR(m_physicalDevice, m_surface, &presentModeCount, presentModes.data());

    VkPresentModeKHR presentMode = VK_PRESENT_MODE_FIFO_KHR; // Guaranteed
    for (auto pm : presentModes) {
        if (pm == VK_PRESENT_MODE_MAILBOX_KHR) { presentMode = pm; break; }
    }

    m_swapChainExtent = caps.currentExtent;
    if (m_swapChainExtent.width == std::numeric_limits<u32>::max()) {
        m_swapChainExtent.width  = std::max(caps.minImageExtent.width,
                                            std::min(caps.maxImageExtent.width, width));
        m_swapChainExtent.height = std::max(caps.minImageExtent.height,
                                            std::min(caps.maxImageExtent.height, height));
    }

    u32 imageCount = caps.minImageCount + 1;
    if (caps.maxImageCount > 0 && imageCount > caps.maxImageCount)
        imageCount = caps.maxImageCount;

    VkSwapchainCreateInfoKHR ci = {VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR};
    ci.surface          = m_surface;
    ci.minImageCount    = imageCount;
    ci.imageFormat      = m_swapChainFormat;
    ci.imageColorSpace  = surfaceFormat.colorSpace;
    ci.imageExtent      = m_swapChainExtent;
    ci.imageArrayLayers = 1;
    ci.imageUsage       = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    u32 queueFamilyIndices[] = { m_graphicsFamily, m_presentFamily };
    if (m_graphicsFamily != m_presentFamily) {
        ci.imageSharingMode      = VK_SHARING_MODE_CONCURRENT;
        ci.queueFamilyIndexCount = 2;
        ci.pQueueFamilyIndices   = queueFamilyIndices;
    } else {
        ci.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }

    ci.preTransform   = caps.currentTransform;
    ci.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    ci.presentMode    = presentMode;
    ci.clipped        = VK_TRUE;
    ci.oldSwapchain   = VK_NULL_HANDLE;

    if (vkCreateSwapchainKHR(m_device, &ci, nullptr, &m_swapChain) != VK_SUCCESS) {
        LOG_ERROR("vkCreateSwapchainKHR failed");
        return false;
    }

    // Get images
    vkGetSwapchainImagesKHR(m_device, m_swapChain, &imageCount, nullptr);
    m_swapChainImages.resize(imageCount);
    vkGetSwapchainImagesKHR(m_device, m_swapChain, &imageCount, m_swapChainImages.data());

    // Create image views
    m_swapChainImageViews.resize(imageCount);
    for (size_t i = 0; i < imageCount; ++i) {
        VkImageViewCreateInfo vci = {VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
        vci.image    = m_swapChainImages[i];
        vci.viewType = VK_IMAGE_VIEW_TYPE_2D;
        vci.format   = m_swapChainFormat;
        vci.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
        vkCreateImageView(m_device, &vci, nullptr, &m_swapChainImageViews[i]);
    }

    // Depth resources
    {
        VkImageCreateInfo ici = {VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
        ici.imageType     = VK_IMAGE_TYPE_2D;
        ici.extent        = { m_swapChainExtent.width, m_swapChainExtent.height, 1 };
        ici.mipLevels     = 1;
        ici.arrayLayers   = 1;
        ici.format        = VK_FORMAT_D32_SFLOAT;
        ici.tiling        = VK_IMAGE_TILING_OPTIMAL;
        ici.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        ici.usage         = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
        ici.samples       = VK_SAMPLE_COUNT_1_BIT;
        ici.sharingMode   = VK_SHARING_MODE_EXCLUSIVE;

        vkCreateImage(m_device, &ici, nullptr, &m_depthImage);

        VkMemoryRequirements memReq;
        vkGetImageMemoryRequirements(m_device, m_depthImage, &memReq);

        VkMemoryAllocateInfo ai = {VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
        ai.allocationSize  = memReq.size;
        ai.memoryTypeIndex = FindMemoryType(memReq.memoryTypeBits,
                                            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        vkAllocateMemory(m_device, &ai, nullptr, &m_depthImageMemory);
        vkBindImageMemory(m_device, m_depthImage, m_depthImageMemory, 0);

        VkImageViewCreateInfo vci = {VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
        vci.image    = m_depthImage;
        vci.viewType = VK_IMAGE_VIEW_TYPE_2D;
        vci.format   = VK_FORMAT_D32_SFLOAT;
        vci.subresourceRange = {VK_IMAGE_ASPECT_DEPTH_BIT, 0, 1, 0, 1};
        vkCreateImageView(m_device, &vci, nullptr, &m_depthImageView);

        TransitionImageLayout(m_depthImage, VK_FORMAT_D32_SFLOAT,
                              VK_IMAGE_LAYOUT_UNDEFINED,
                              VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
    }

    m_width  = m_swapChainExtent.width;
    m_height = m_swapChainExtent.height;
    LOG_INFO("Swap chain created: {}x{} x{}", m_width, m_height, imageCount);
    return true;
}

void VulkanRenderer::DestroySwapChain() {
    vkDeviceWaitIdle(m_device);
    for (auto& fb : m_swapChainFramebuffers) {
        vkDestroyFramebuffer(m_device, fb, nullptr);
    }
    m_swapChainFramebuffers.clear();

    for (auto& iv : m_swapChainImageViews) {
        vkDestroyImageView(m_device, iv, nullptr);
    }
    m_swapChainImageViews.clear();

    vkDestroyImageView(m_device, m_depthImageView, nullptr);
    vkDestroyImage(m_device, m_depthImage, nullptr);
    vkFreeMemory(m_device, m_depthImageMemory, nullptr);

    vkDestroySwapchainKHR(m_device, m_swapChain, nullptr);
    m_swapChain = VK_NULL_HANDLE;
}

// ==================================================================
// Render pass
// ==================================================================
bool VulkanRenderer::CreateRenderPass() {
    VkAttachmentDescription colorAttachment = {};
    colorAttachment.format         = m_swapChainFormat;
    colorAttachment.samples        = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout    = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentDescription depthAttachment = {};
    depthAttachment.format         = VK_FORMAT_D32_SFLOAT;
    depthAttachment.samples        = VK_SAMPLE_COUNT_1_BIT;
    depthAttachment.loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp        = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
    depthAttachment.finalLayout    = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference colorRef = {};
    colorRef.attachment = 0;
    colorRef.layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depthRef = {};
    depthRef.attachment = 1;
    depthRef.layout     = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint    = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments    = &colorRef;
    subpass.pDepthStencilAttachment = &depthRef;

    VkSubpassDependency dep = {};
    dep.srcSubpass    = VK_SUBPASS_EXTERNAL;
    dep.dstSubpass    = 0;
    dep.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
                       VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dep.srcAccessMask = 0;
    dep.dstStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
                        VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dep.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT |
                        VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    VkAttachmentDescription attachments[] = { colorAttachment, depthAttachment };

    VkRenderPassCreateInfo rpci = {VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};
    rpci.attachmentCount = 2;
    rpci.pAttachments    = attachments;
    rpci.subpassCount    = 1;
    rpci.pSubpasses     = &subpass;
    rpci.dependencyCount = 1;
    rpci.pDependencies   = &dep;

    if (vkCreateRenderPass(m_device, &rpci, nullptr, &m_renderPass) != VK_SUCCESS) {
        LOG_ERROR("vkCreateRenderPass failed");
        return false;
    }
    return true;
}

// ==================================================================
// Descriptor pool
// ==================================================================
bool VulkanRenderer::CreateDescriptorPool() {
    VkDescriptorPoolSize poolSize = {};
    poolSize.type            = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSize.descriptorCount = 64; // enough for many pipelines

    VkDescriptorPoolCreateInfo ci = {VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
    ci.maxSets       = 64;
    ci.poolSizeCount = 1;
    ci.pPoolSizes    = &poolSize;

    if (vkCreateDescriptorPool(m_device, &ci, nullptr, &m_descriptorPool) != VK_SUCCESS) {
        LOG_ERROR("vkCreateDescriptorPool failed");
        return false;
    }
    return true;
}

// ==================================================================
// Command pool
// ==================================================================
bool VulkanRenderer::CreateCommandPool() {
    VkCommandPoolCreateInfo ci = {VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
    ci.flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    ci.queueFamilyIndex = m_graphicsFamily;

    if (vkCreateCommandPool(m_device, &ci, nullptr, &m_commandPool) != VK_SUCCESS) {
        LOG_ERROR("vkCreateCommandPool failed");
        return false;
    }
    return true;
}

// ==================================================================
// Sync objects
// ==================================================================
bool VulkanRenderer::CreateSyncObjects() {
    VkSemaphoreCreateInfo sci = {VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
    VkFenceCreateInfo fci = {VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
    fci.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    if (vkCreateSemaphore(m_device, &sci, nullptr, &m_imageAvailableSemaphore) != VK_SUCCESS ||
        vkCreateSemaphore(m_device, &sci, nullptr, &m_renderFinishedSemaphore) != VK_SUCCESS ||
        vkCreateFence(m_device, &fci, nullptr, &m_inFlightFence) != VK_SUCCESS) {
        LOG_ERROR("Sync object creation failed");
        return false;
    }
    return true;
}

// ==================================================================
// Framebuffers
// ==================================================================
bool VulkanRenderer::CreateFramebuffers() {
    m_swapChainFramebuffers.resize(m_swapChainImageViews.size());
    for (size_t i = 0; i < m_swapChainImageViews.size(); ++i) {
        VkImageView attachments[] = { m_swapChainImageViews[i], m_depthImageView };

        VkFramebufferCreateInfo ci = {VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
        ci.renderPass      = m_renderPass;
        ci.attachmentCount = 2;
        ci.pAttachments    = attachments;
        ci.width           = m_swapChainExtent.width;
        ci.height          = m_swapChainExtent.height;
        ci.layers          = 1;

        if (vkCreateFramebuffer(m_device, &ci, nullptr, &m_swapChainFramebuffers[i]) != VK_SUCCESS) {
            LOG_ERROR("Framebuffer {} creation failed", i);
            return false;
        }
    }
    return true;
}

// ==================================================================
// Shutdown
// ==================================================================
void VulkanRenderer::Shutdown() {
    if (!m_initialized) return;
    vkDeviceWaitIdle(m_device);

    m_activePipeline = nullptr;

    // Destroy meshes
    for (auto& m : m_meshes) {
        vkDestroyBuffer(m_device, m.vertexBuffer, nullptr);
        vkFreeMemory(m_device, m.vertexMemory, nullptr);
        if (m.indexBuffer) {
            vkDestroyBuffer(m_device, m.indexBuffer, nullptr);
            vkFreeMemory(m_device, m.indexMemory, nullptr);
        }
    }
    m_meshes.clear();

    // Destroy pipelines
    for (auto& p : m_pipelines) {
        vkDestroyPipeline(m_device, p.pipeline, nullptr);
        vkDestroyPipelineLayout(m_device, p.pipelineLayout, nullptr);
        vkDestroyDescriptorSetLayout(m_device, p.descSetLayout, nullptr);
        vkDestroyShaderModule(m_device, p.vsModule, nullptr);
        vkDestroyShaderModule(m_device, p.psModule, nullptr);
        if (p.uniformBuffer) {
            vkDestroyBuffer(m_device, p.uniformBuffer, nullptr);
            vkFreeMemory(m_device, p.uniformMemory, nullptr);
        }
    }
    m_pipelines.clear();

    // Destroy textures
    for (auto& t : m_textures) {
        vkDestroySampler(m_device, t.sampler, nullptr);
        vkDestroyImageView(m_device, t.imageView, nullptr);
        vkDestroyImage(m_device, t.image, nullptr);
        vkFreeMemory(m_device, t.memory, nullptr);
    }
    m_textures.clear();

    // Destroy buffers
    for (auto& b : m_buffers) {
        vkDestroyBuffer(m_device, b.buffer, nullptr);
        vkFreeMemory(m_device, b.memory, nullptr);
    }
    m_buffers.clear();

    // Destroy shader modules
    for (auto& s : m_shaders) {
        vkDestroyShaderModule(m_device, s.module, nullptr);
    }
    m_shaders.clear();

    // Sync objects
    vkDestroySemaphore(m_device, m_imageAvailableSemaphore, nullptr);
    vkDestroySemaphore(m_device, m_renderFinishedSemaphore, nullptr);
    vkDestroyFence(m_device, m_inFlightFence, nullptr);

    vkDestroyCommandPool(m_device, m_commandPool, nullptr);
    vkDestroyDescriptorPool(m_device, m_descriptorPool, nullptr);

    DestroySwapChain();
    vkDestroyRenderPass(m_device, m_renderPass, nullptr);
    vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
    vkDestroyDevice(m_device, nullptr);
    vkDestroyInstance(m_instance, nullptr);

    m_initialized = false;
    LOG_INFO("Vulkan shut down");
}

// ==================================================================
// Frame control
// ==================================================================
void VulkanRenderer::BeginFrame() {
    vkWaitForFences(m_device, 1, &m_inFlightFence, VK_TRUE, UINT64_MAX);
    vkResetFences(m_device, 1, &m_inFlightFence);

    u32 imageIndex;
    VkResult result = vkAcquireNextImageKHR(m_device, m_swapChain, UINT64_MAX,
                                             m_imageAvailableSemaphore, VK_NULL_HANDLE,
                                             &imageIndex);
    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        OnResize(m_width, m_height);
        return;
    }

    // Reset command buffer and begin render pass
    vkResetCommandBuffer(m_commandBuffer, 0);

    VkCommandBufferBeginInfo bi = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    bi.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(m_commandBuffer, &bi);

    VkClearValue clearValues[2];
    clearValues[0].color        = { { 0.5f, 0.7f, 0.95f, 1.0f } };
    clearValues[1].depthStencil = { 1.0f, 0 };

    VkRenderPassBeginInfo rpbi = {VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
    rpbi.renderPass      = m_renderPass;
    rpbi.framebuffer     = m_swapChainFramebuffers[imageIndex];
    rpbi.renderArea      = { {0, 0}, m_swapChainExtent };
    rpbi.clearValueCount = 2;
    rpbi.pClearValues    = clearValues;

    vkCmdBeginRenderPass(m_commandBuffer, &rpbi, VK_SUBPASS_CONTENTS_INLINE);

    // Store current frame index for Present()
    m_currentFrame = imageIndex;
}

void VulkanRenderer::EndFrame() {
    FlushUniforms();
    vkCmdEndRenderPass(m_commandBuffer);
    vkEndCommandBuffer(m_commandBuffer);

    VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

    VkSubmitInfo si = {VK_STRUCTURE_TYPE_SUBMIT_INFO};
    si.waitSemaphoreCount   = 1;
    si.pWaitSemaphores      = &m_imageAvailableSemaphore;
    si.pWaitDstStageMask    = &waitStage;
    si.commandBufferCount   = 1;
    si.pCommandBuffers      = &m_commandBuffer;
    si.signalSemaphoreCount = 1;
    si.pSignalSemaphores    = &m_renderFinishedSemaphore;

    vkQueueSubmit(m_graphicsQueue, 1, &si, m_inFlightFence);
}

void VulkanRenderer::Present() {
    VkPresentInfoKHR pi = {VK_STRUCTURE_TYPE_PRESENT_INFO_KHR};
    pi.waitSemaphoreCount = 1;
    pi.pWaitSemaphores    = &m_renderFinishedSemaphore;
    pi.swapchainCount     = 1;
    pi.pSwapchains        = &m_swapChain;
    pi.pImageIndices      = &m_currentFrame;

    VkResult result = vkQueuePresentKHR(m_presentQueue, &pi);
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
        OnResize(m_width, m_height);
    }
}

void VulkanRenderer::ClearColor(f32 r, f32 g, f32 b, f32 a) {
    VkClearAttachment att = {};
    att.aspectMask      = VK_IMAGE_ASPECT_COLOR_BIT;
    att.clearValue.color = { { r, g, b, a } };
    att.colorAttachment = 0;

    VkClearRect rect = {};
    rect.rect        = { {0, 0}, m_swapChainExtent };
    rect.baseArrayLayer = 0;
    rect.layerCount     = 1;

    vkCmdClearAttachments(m_commandBuffer, 1, &att, 1, &rect);
}

void VulkanRenderer::ClearDepth(f32 value) {
    VkClearAttachment att = {};
    att.aspectMask        = VK_IMAGE_ASPECT_DEPTH_BIT;
    att.clearValue.depthStencil = { value, 0 };

    VkClearRect rect = {};
    rect.rect        = { {0, 0}, m_swapChainExtent };
    rect.baseArrayLayer = 0;
    rect.layerCount     = 1;

    vkCmdClearAttachments(m_commandBuffer, 1, &att, 1, &rect);
}

void VulkanRenderer::SetViewport(const ViewportDesc& d) {
    VkViewport vp = { d.x, d.y + d.height, d.width, -d.height, d.minDepth, d.maxDepth };
    vkCmdSetViewport(m_commandBuffer, 0, 1, &vp);

    VkRect2D scissor = { {(i32)d.x, (i32)d.y}, {(u32)d.width, (u32)d.height} };
    vkCmdSetScissor(m_commandBuffer, 0, 1, &scissor);
}

// ==================================================================
// Create resources
// ==================================================================
ShaderHandle VulkanRenderer::CreateShader(const ShaderDesc& desc) {
    VkShader s;
    s.stage  = desc.stage;
    s.handle = (ShaderHandle)m_shaders.size();

    std::vector<u32> spirv;
    s.module = CompileToSPIRV(desc.stage, desc.source, spirv);
    if (!s.module) return kInvalidHandle;

    s.spirv = std::move(spirv);
    m_shaders.push_back(std::move(s));
    return (ShaderHandle)(m_shaders.size() - 1);
}

BufferHandle VulkanRenderer::CreateBuffer(const BufferDesc& desc) {
    painkiller::VkBuffer b;
    b.type   = desc.type;
    b.size   = desc.size;
    b.stride = desc.stride;

    VkBufferUsageFlags usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    VkMemoryPropertyFlags memProps = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

    switch (desc.type) {
        case BufferType::Vertex:
            usage |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
            break;
        case BufferType::Index:
            usage |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
            break;
        case BufferType::Constant:
            usage |= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
            memProps = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                       VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
            break;
        default:
            usage |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
            break;
    }

    CreateVkBuffer(desc.size, usage, memProps, b.buffer, b.memory, desc.initialData);
    m_buffers.push_back(std::move(b));
    return (BufferHandle)(m_buffers.size() - 1);
}

TextureHandle VulkanRenderer::CreateTexture(const TextureDesc& desc) {
    VkTexture t;
    t.width  = desc.width;
    t.height = desc.height;
    t.format = desc.format;

    VkFormat vkFormat = ToVkFormat(desc.format);

    // Create image
    VkImageCreateInfo ici = {VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
    ici.imageType     = VK_IMAGE_TYPE_2D;
    ici.extent        = { desc.width, desc.height, 1 };
    ici.mipLevels     = desc.mipLevels;
    ici.arrayLayers   = 1;
    ici.format        = vkFormat;
    ici.tiling        = VK_IMAGE_TILING_OPTIMAL;
    ici.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    ici.usage         = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    ici.samples       = VK_SAMPLE_COUNT_1_BIT;
    ici.sharingMode   = VK_SHARING_MODE_EXCLUSIVE;
    if (desc.mipLevels > 1) ici.usage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;

    vkCreateImage(m_device, &ici, nullptr, &t.image);

    VkMemoryRequirements memReq;
    vkGetImageMemoryRequirements(m_device, t.image, &memReq);

    VkMemoryAllocateInfo ai = {VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
    ai.allocationSize  = memReq.size;
    ai.memoryTypeIndex = FindMemoryType(memReq.memoryTypeBits,
                                        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    vkAllocateMemory(m_device, &ai, nullptr, &t.memory);
    vkBindImageMemory(m_device, t.image, t.memory, 0);

    // Upload data if provided
    if (desc.initialData) {
        VkDeviceSize imageSize = desc.width * desc.height * 4;
        VkBuffer stagingBuffer;
        VkDeviceMemory stagingMemory;
        CreateVkBuffer(imageSize,
                       VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                       VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                       stagingBuffer, stagingMemory, desc.initialData);

        TransitionImageLayout(t.image, vkFormat,
                              VK_IMAGE_LAYOUT_UNDEFINED,
                              VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
        CopyBufferToImage(stagingBuffer, t.image, desc.width, desc.height);
        TransitionImageLayout(t.image, vkFormat,
                              VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                              VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

        vkDestroyBuffer(m_device, stagingBuffer, nullptr);
        vkFreeMemory(m_device, stagingMemory, nullptr);
    } else {
        TransitionImageLayout(t.image, vkFormat,
                              VK_IMAGE_LAYOUT_UNDEFINED,
                              VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    }

    // Image view
    VkImageViewCreateInfo vci = {VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
    vci.image    = t.image;
    vci.viewType = VK_IMAGE_VIEW_TYPE_2D;
    vci.format   = vkFormat;
    vci.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, desc.mipLevels, 0, 1};
    vkCreateImageView(m_device, &vci, nullptr, &t.imageView);

    // Sampler
    VkSamplerCreateInfo sci = {VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
    sci.magFilter        = VK_FILTER_LINEAR;
    sci.minFilter        = VK_FILTER_LINEAR;
    sci.mipmapMode       = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    sci.addressModeU     = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sci.addressModeV     = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sci.addressModeW     = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sci.mipLodBias       = 0.0f;
    sci.anisotropyEnable = VK_FALSE;
    sci.minLod           = 0.0f;
    sci.maxLod           = (f32)desc.mipLevels;
    vkCreateSampler(m_device, &sci, nullptr, &t.sampler);

    m_textures.push_back(std::move(t));
    return (TextureHandle)(m_textures.size() - 1);
}

PipelineHandle VulkanRenderer::CreatePipelineState(const PipelineStateDesc& desc) {
    painkiller::VkPipeline p;
    p.topology = desc.topology;

    // --- Compile shaders ---
    if (desc.vertexShader) {
        std::vector<u32> spirv;
        p.vsModule = CompileToSPIRV(ShaderStage::Vertex, desc.vertexShader->source, spirv);
        if (!p.vsModule) return kInvalidHandle;
        ReflectUniforms(spirv, p.vsSlots, p.uniformSize);
    }
    if (desc.fragmentShader) {
        std::vector<u32> spirv;
        p.psModule = CompileToSPIRV(ShaderStage::Fragment, desc.fragmentShader->source, spirv);
        if (!p.psModule) return kInvalidHandle;
        ReflectUniforms(spirv, p.psSlots, p.uniformSize);
    }

    // --- Descriptor set layout (UBO binding 0) ---
    VkDescriptorSetLayoutBinding dslBinding = {};
    dslBinding.binding         = 0;
    dslBinding.descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    dslBinding.descriptorCount = 1;
    dslBinding.stageFlags      = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutCreateInfo dslci = {VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
    dslci.bindingCount = 1;
    dslci.pBindings    = &dslBinding;
    vkCreateDescriptorSetLayout(m_device, &dslci, nullptr, &p.descSetLayout);

    // --- Pipeline layout ---
    VkPipelineLayoutCreateInfo plci = {VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
    plci.setLayoutCount = 1;
    plci.pSetLayouts    = &p.descSetLayout;
    vkCreatePipelineLayout(m_device, &plci, nullptr, &p.pipelineLayout);

    // --- Allocate descriptor set ---
    VkDescriptorSetAllocateInfo dsai = {VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
    dsai.descriptorPool     = m_descriptorPool;
    dsai.descriptorSetCount = 1;
    dsai.pSetLayouts        = &p.descSetLayout;
    vkAllocateDescriptorSets(m_device, &dsai, &p.descSet);

    // --- Create UBO if shaders have uniforms ---
    if (p.uniformSize > 0) {
        u32 alignedSize = (p.uniformSize + 15) & ~15;
        if (alignedSize < 16) alignedSize = 16; // minimum UBO size

        CreateVkBuffer(alignedSize,
                       VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                       VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                           VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                       p.uniformBuffer, p.uniformMemory);

        p.shadow.resize(alignedSize, 0);
        p.uniformSize = alignedSize;

        // Update descriptor set
        VkDescriptorBufferInfo bufferInfo = {};
        bufferInfo.buffer = p.uniformBuffer;
        bufferInfo.range  = alignedSize;

        VkWriteDescriptorSet wds = {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
        wds.dstSet          = p.descSet;
        wds.dstBinding      = 0;
        wds.descriptorCount = 1;
        wds.descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        wds.pBufferInfo     = &bufferInfo;
        vkUpdateDescriptorSets(m_device, 1, &wds, 0, nullptr);
    }

    // --- Shader stage info ---
    std::array<VkPipelineShaderStageCreateInfo, 2> stages = {};
    u32 stageCount = 0;

    if (p.vsModule) {
        stages[stageCount] = {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO};
        stages[stageCount].stage  = VK_SHADER_STAGE_VERTEX_BIT;
        stages[stageCount].module = p.vsModule;
        stages[stageCount].pName  = "main";
        stageCount++;
    }
    if (p.psModule) {
        stages[stageCount] = {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO};
        stages[stageCount].stage  = VK_SHADER_STAGE_FRAGMENT_BIT;
        stages[stageCount].module = p.psModule;
        stages[stageCount].pName  = "main";
        stageCount++;
    }

    // --- Vertex input ---
    std::vector<VkVertexInputBindingDescription> bindDescs;
    std::vector<VkVertexInputAttributeDescription> attrDescs;
    u32 vertexStride = 0;

    if (!desc.vertexLayout.empty()) {
        // Determine stride by looking at last element's offset + its size
        for (const auto& e : desc.vertexLayout) {
            u32 offset = e.offset + 4; // minimum: float
            switch (e.format) {
                case Format::R32_FLOAT:              offset = e.offset + 4;  break;
                case Format::R32G32_FLOAT:           offset = e.offset + 8;  break;
                case Format::R32G32B32_FLOAT:        offset = e.offset + 12; break;
                case Format::R32G32B32A32_FLOAT:     offset = e.offset + 16; break;
                case Format::R8G8B8A8_UNORM:
                case Format::R8G8B8A8_SRGB:          offset = e.offset + 4;  break;
                default: break;
            }
            if (offset > vertexStride) vertexStride = offset;
        }

        VkVertexInputBindingDescription bd = {};
        bd.binding   = 0;
        bd.stride    = vertexStride;
        bd.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        bindDescs.push_back(bd);

        for (size_t i = 0; i < desc.vertexLayout.size(); ++i) {
            const auto& e = desc.vertexLayout[i];
            VkFormat vf = ToVkFormat(e.format);
            VkVertexInputAttributeDescription ad = {};
            ad.location = (u32)i;
            ad.binding  = e.slot;
            ad.format   = vf;
            ad.offset   = e.offset;
            attrDescs.push_back(ad);
        }
    }

    VkPipelineVertexInputStateCreateInfo visci = {VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};
    visci.vertexBindingDescriptionCount   = (u32)bindDescs.size();
    visci.pVertexBindingDescriptions      = bindDescs.data();
    visci.vertexAttributeDescriptionCount = (u32)attrDescs.size();
    visci.pVertexAttributeDescriptions    = attrDescs.data();

    // --- Input assembly ---
    VkPipelineInputAssemblyStateCreateInfo iasci = {VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};
    iasci.topology               = ToVkTopology(desc.topology);
    iasci.primitiveRestartEnable = VK_FALSE;

    // --- Viewport ---
    VkPipelineViewportStateCreateInfo vsci = {VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};
    vsci.viewportCount = 1;
    vsci.scissorCount  = 1;

    // --- Rasterizer ---
    VkPipelineRasterizationStateCreateInfo rsci = {VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};
    rsci.depthClampEnable        = VK_FALSE;
    rsci.rasterizerDiscardEnable = VK_FALSE;
    rsci.polygonMode             = ToVkFillMode(desc.fillMode);
    rsci.cullMode                = ToVkCullMode(desc.cullMode);
    rsci.frontFace               = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rsci.depthBiasEnable         = VK_FALSE;
    rsci.lineWidth               = 1.0f;

    // --- Multisampling ---
    VkPipelineMultisampleStateCreateInfo msci = {VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};
    msci.rasterizationSamples  = VK_SAMPLE_COUNT_1_BIT;
    msci.sampleShadingEnable   = VK_FALSE;

    // --- Depth/stencil ---
    VkPipelineDepthStencilStateCreateInfo dsci = {VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO};
    dsci.depthTestEnable       = desc.depthTestEnabled ? VK_TRUE : VK_FALSE;
    dsci.depthWriteEnable      = desc.depthWriteEnabled ? VK_TRUE : VK_FALSE;
    dsci.depthCompareOp        = ToVkCompare(desc.depthFunc);
    dsci.depthBoundsTestEnable = VK_FALSE;
    dsci.stencilTestEnable     = VK_FALSE;

    // --- Color blend ---
    VkPipelineColorBlendAttachmentState cbas = {};
    cbas.blendEnable         = (desc.blendMode != BlendMode::Opaque) ? VK_TRUE : VK_FALSE;
    cbas.srcColorBlendFactor = ToVkSrcBlend(desc.blendMode);
    cbas.dstColorBlendFactor = ToVkDstBlend(desc.blendMode);
    cbas.colorBlendOp        = VK_BLEND_OP_ADD;
    cbas.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    cbas.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    cbas.alphaBlendOp        = VK_BLEND_OP_ADD;
    cbas.colorWriteMask      = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                                VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

    VkPipelineColorBlendStateCreateInfo cbsci = {VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};
    cbsci.logicOpEnable   = VK_FALSE;
    cbsci.attachmentCount = 1;
    cbsci.pAttachments    = &cbas;

    // --- Dynamic state (viewport, scissor) ---
    VkDynamicState dynStates[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
    VkPipelineDynamicStateCreateInfo dsci2 = {VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO};
    dsci2.dynamicStateCount = 2;
    dsci2.pDynamicStates    = dynStates;

    // --- Create pipeline ---
    VkGraphicsPipelineCreateInfo gpci = {VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};
    gpci.stageCount          = stageCount;
    gpci.pStages             = stages.data();
    gpci.pVertexInputState   = &visci;
    gpci.pInputAssemblyState = &iasci;
    gpci.pViewportState      = &vsci;
    gpci.pRasterizationState = &rsci;
    gpci.pMultisampleState   = &msci;
    gpci.pDepthStencilState  = &dsci;
    gpci.pColorBlendState    = &cbsci;
    gpci.pDynamicState       = &dsci2;
    gpci.layout              = p.pipelineLayout;
    gpci.renderPass          = m_renderPass;
    gpci.subpass             = 0;

    if (vkCreateGraphicsPipelines(m_device, VK_NULL_HANDLE, 1, &gpci, nullptr, &p.pipeline) != VK_SUCCESS) {
        LOG_ERROR("vkCreateGraphicsPipelines failed");
        return kInvalidHandle;
    }

    m_pipelines.push_back(std::move(p));
    return (PipelineHandle)(m_pipelines.size() - 1);
}

MeshHandle VulkanRenderer::CreateMesh(const MeshData& data) {
    VkMesh m;
    m.vertexCount  = data.vertexCount;
    m.indexCount   = data.indexCount;
    m.vertexStride = data.vertexStride;
    m.indexed      = data.indexCount > 0;

    VkDeviceSize vsize = data.vertices.size() * sizeof(f32);
    if (vsize > 0) {
        CreateVkBuffer(vsize,
                       VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                       VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                       m.vertexBuffer, m.vertexMemory);

        // Staging buffer
        VkBuffer stagingBuf;
        VkDeviceMemory stagingMem;
        CreateVkBuffer(vsize,
                       VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                       VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                       stagingBuf, stagingMem, data.vertices.data());

        copyBuffer(m_device, m_commandPool, m_graphicsQueue, m.vertexBuffer, stagingBuf, vsize);
        vkDestroyBuffer(m_device, stagingBuf, nullptr);
        vkFreeMemory(m_device, stagingMem, nullptr);
    }

    if (m.indexed) {
        VkDeviceSize isize = data.indices.size() * sizeof(u32);
        CreateVkBuffer(isize,
                       VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                       VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                       m.indexBuffer, m.indexMemory);

        VkBuffer stagingBuf;
        VkDeviceMemory stagingMem;
        CreateVkBuffer(isize,
                       VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                       VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                       stagingBuf, stagingMem, data.indices.data());

        copyBuffer(m_device, m_commandPool, m_graphicsQueue, m.indexBuffer, stagingBuf, isize);
        vkDestroyBuffer(m_device, stagingBuf, nullptr);
        vkFreeMemory(m_device, stagingMem, nullptr);
    }

    m_meshes.push_back(std::move(m));
    return (MeshHandle)(m_meshes.size() - 1);
}

// Need to add the helper method copyBuffer
// Adding it as a private helper in the cpp

static void copyBuffer(VkDevice device, VkCommandPool commandPool, VkQueue queue,
                        VkBuffer dst, VkBuffer src, VkDeviceSize size)
{
    VkCommandBufferAllocateInfo ai = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
    ai.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    ai.commandPool = commandPool;
    ai.commandBufferCount = 1;

    VkCommandBuffer cmd;
    vkAllocateCommandBuffers(device, &ai, &cmd);

    VkCommandBufferBeginInfo bi = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    bi.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(cmd, &bi);

    VkBufferCopy copy = {};
    copy.size = size;
    vkCmdCopyBuffer(cmd, src, dst, 1, &copy);

    vkEndCommandBuffer(cmd);

    VkSubmitInfo si = {VK_STRUCTURE_TYPE_SUBMIT_INFO};
    si.commandBufferCount = 1;
    si.pCommandBuffers = &cmd;
    vkQueueSubmit(queue, 1, &si, VK_NULL_HANDLE);
    vkQueueWaitIdle(queue);

    vkFreeCommandBuffers(device, commandPool, 1, &cmd);
}

// ==================================================================
// Destroy resources
// ==================================================================
void VulkanRenderer::DestroyShader(ShaderHandle h) {
    if (h < m_shaders.size()) {
        vkDestroyShaderModule(m_device, m_shaders[h].module, nullptr);
        m_shaders[h] = VkShader{};
    }
}

void VulkanRenderer::DestroyBuffer(BufferHandle h) {
    if (h < m_buffers.size()) {
        vkDestroyBuffer(m_device, m_buffers[h].buffer, nullptr);
        vkFreeMemory(m_device, m_buffers[h].memory, nullptr);
        m_buffers[h] = painkiller::VkBuffer{};
    }
}

void VulkanRenderer::DestroyTexture(TextureHandle h) {
    if (h < m_textures.size()) {
        vkDestroySampler(m_device, m_textures[h].sampler, nullptr);
        vkDestroyImageView(m_device, m_textures[h].imageView, nullptr);
        vkDestroyImage(m_device, m_textures[h].image, nullptr);
        vkFreeMemory(m_device, m_textures[h].memory, nullptr);
        m_textures[h] = VkTexture{};
    }
}

void VulkanRenderer::DestroyPipelineState(PipelineHandle h) {
    if (h < m_pipelines.size()) {
        auto& p = m_pipelines[h];
        vkDestroyPipeline(m_device, p.pipeline, nullptr);
        vkDestroyPipelineLayout(m_device, p.pipelineLayout, nullptr);
        vkDestroyDescriptorSetLayout(m_device, p.descSetLayout, nullptr);
        vkDestroyShaderModule(m_device, p.vsModule, nullptr);
        vkDestroyShaderModule(m_device, p.psModule, nullptr);
        if (p.uniformBuffer) {
            vkDestroyBuffer(m_device, p.uniformBuffer, nullptr);
            vkFreeMemory(m_device, p.uniformMemory, nullptr);
        }
        m_pipelines[h] = painkiller::VkPipeline{};
    }
}

void VulkanRenderer::DestroyMesh(MeshHandle h) {
    if (h < m_meshes.size()) {
        auto& m = m_meshes[h];
        vkDestroyBuffer(m_device, m.vertexBuffer, nullptr);
        vkFreeMemory(m_device, m.vertexMemory, nullptr);
        if (m.indexBuffer) {
            vkDestroyBuffer(m_device, m.indexBuffer, nullptr);
            vkFreeMemory(m_device, m.indexMemory, nullptr);
        }
        m_meshes[h] = VkMesh{};
    }
}

// ==================================================================
// Binding
// ==================================================================
void VulkanRenderer::BindPipelineState(PipelineHandle h) {
    if (h >= m_pipelines.size()) return;
    m_activePipeline = &m_pipelines[h];
    m_activePipelineLayout = m_activePipeline->pipelineLayout;

    vkCmdBindPipeline(m_commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                      m_activePipeline->pipeline);
    vkCmdBindDescriptorSets(m_commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                            m_activePipelineLayout, 0, 1,
                            &m_activePipeline->descSet, 0, nullptr);
}

void VulkanRenderer::BindVertexBuffer(BufferHandle handle, u32 slot) {
    if (handle >= m_buffers.size()) return;
    VkBuffer buf = m_buffers[handle].buffer;
    VkDeviceSize offset = 0;
    vkCmdBindVertexBuffers(m_commandBuffer, slot, 1, &buf, &offset);
}

void VulkanRenderer::BindIndexBuffer(BufferHandle handle) {
    if (handle >= m_buffers.size()) return;
    vkCmdBindIndexBuffer(m_commandBuffer, m_buffers[handle].buffer, 0, VK_INDEX_TYPE_UINT32);
}

void VulkanRenderer::BindTexture(TextureHandle handle, u32 slot) {
    if (handle >= m_textures.size()) return;
    // Bind via descriptor set — for simplicity we use push descriptor or
    // inline descriptor updates. In a full engine, this would use a
    // per-frame descriptor set. For now, we write directly to the pipeline's
    // descriptor set.
    if (m_activePipeline) {
        VkDescriptorImageInfo imageInfo = {};
        imageInfo.imageView   = m_textures[handle].imageView;
        imageInfo.sampler     = m_textures[handle].sampler;
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        VkWriteDescriptorSet wds = {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
        wds.dstSet          = m_activePipeline->descSet;
        wds.dstBinding      = slot;
        wds.descriptorCount = 1;
        wds.descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        wds.pImageInfo     = &imageInfo;
        vkUpdateDescriptorSets(m_device, 1, &wds, 0, nullptr);
    }
}

void VulkanRenderer::BindMesh(MeshHandle handle) {
    if (handle >= m_meshes.size()) return;
    auto& m = m_meshes[handle];
    VkBuffer vb = m.vertexBuffer;
    VkDeviceSize offset = 0;
    vkCmdBindVertexBuffers(m_commandBuffer, 0, 1, &vb, &offset);
    if (m.indexBuffer) {
        vkCmdBindIndexBuffer(m_commandBuffer, m.indexBuffer, 0, VK_INDEX_TYPE_UINT32);
    }
}

// ==================================================================
// Uniform setting
// ==================================================================
void VulkanRenderer::FlushUniforms() {
    if (!m_activePipeline || !m_activePipeline->uniformBuffer) return;
    if (!m_activePipeline->dirty) return;

    void* mapped;
    vkMapMemory(m_device, m_activePipeline->uniformMemory, 0,
                m_activePipeline->uniformSize, 0, &mapped);
    memcpy(mapped, m_activePipeline->shadow.data(), m_activePipeline->shadow.size());
    vkUnmapMemory(m_device, m_activePipeline->uniformMemory);
    m_activePipeline->dirty = false;
}

static bool FindVkUniformSlot(const std::vector<VkReflectionUniform>& slots,
                               const char* name, u32& outOffset, u32& outSize)
{
    for (const auto& s : slots) {
        if (s.name == name) {
            outOffset = s.offset;
            outSize   = s.size;
            return true;
        }
    }
    return false;
}

void VulkanRenderer::SetUniformBuffer(BufferHandle handle, u32 slot) {
    if (handle >= m_buffers.size()) return;
    // In a full implementation, write buffer info to a descriptor set.
    // For simplicity, this is a no-op — the per-pipeline UBO covers named uniforms.
    (void)slot;
}

void VulkanRenderer::SetUniformMat4(const char* name, const Mat4& matrix) {
    if (!m_activePipeline || m_activePipeline->shadow.empty()) return;
    u32 offset, size;
    if (FindVkUniformSlot(m_activePipeline->vsSlots, name, offset, size) ||
        FindVkUniformSlot(m_activePipeline->psSlots, name, offset, size)) {
        if (offset + sizeof(glm::mat4) <= m_activePipeline->shadow.size()) {
            memcpy(&m_activePipeline->shadow[offset], glm::value_ptr(matrix), sizeof(glm::mat4));
            m_activePipeline->dirty = true;
        }
    }
}

void VulkanRenderer::SetUniformVec3(const char* name, const Vec3& value) {
    if (!m_activePipeline || m_activePipeline->shadow.empty()) return;
    u32 offset, size;
    if (FindVkUniformSlot(m_activePipeline->vsSlots, name, offset, size) ||
        FindVkUniformSlot(m_activePipeline->psSlots, name, offset, size)) {
        if (offset + sizeof(glm::vec3) <= m_activePipeline->shadow.size()) {
            memcpy(&m_activePipeline->shadow[offset], glm::value_ptr(value), sizeof(glm::vec3));
            m_activePipeline->dirty = true;
        }
    }
}

void VulkanRenderer::SetUniformVec4(const char* name, const Vec4& value) {
    if (!m_activePipeline || m_activePipeline->shadow.empty()) return;
    u32 offset, size;
    if (FindVkUniformSlot(m_activePipeline->vsSlots, name, offset, size) ||
        FindVkUniformSlot(m_activePipeline->psSlots, name, offset, size)) {
        if (offset + sizeof(glm::vec4) <= m_activePipeline->shadow.size()) {
            memcpy(&m_activePipeline->shadow[offset], glm::value_ptr(value), sizeof(glm::vec4));
            m_activePipeline->dirty = true;
        }
    }
}

void VulkanRenderer::SetUniformFloat(const char* name, f32 value) {
    if (!m_activePipeline || m_activePipeline->shadow.empty()) return;
    u32 offset, size;
    if (FindVkUniformSlot(m_activePipeline->vsSlots, name, offset, size) ||
        FindVkUniformSlot(m_activePipeline->psSlots, name, offset, size)) {
        if (offset + sizeof(f32) <= m_activePipeline->shadow.size()) {
            memcpy(&m_activePipeline->shadow[offset], &value, sizeof(f32));
            m_activePipeline->dirty = true;
        }
    }
}

void VulkanRenderer::SetUniformInt(const char* name, i32 value) {
    if (!m_activePipeline || m_activePipeline->shadow.empty()) return;
    u32 offset, size;
    if (FindVkUniformSlot(m_activePipeline->vsSlots, name, offset, size) ||
        FindVkUniformSlot(m_activePipeline->psSlots, name, offset, size)) {
        if (offset + sizeof(i32) <= m_activePipeline->shadow.size()) {
            memcpy(&m_activePipeline->shadow[offset], &value, sizeof(i32));
            m_activePipeline->dirty = true;
        }
    }
}

// ==================================================================
// Draw
// ==================================================================
void VulkanRenderer::Draw(u32 vertexCount, u32 startVertex) {
    FlushUniforms();
    vkCmdDraw(m_commandBuffer, vertexCount, 1, startVertex, 0);
}

void VulkanRenderer::DrawIndexed(u32 indexCount, u32 startIndex, i32 baseVertex) {
    FlushUniforms();
    vkCmdDrawIndexed(m_commandBuffer, indexCount, 1, startIndex, baseVertex, 0);
}

void VulkanRenderer::DrawMesh(MeshHandle handle) {
    if (handle >= m_meshes.size()) return;
    FlushUniforms();

    auto& m = m_meshes[handle];
    VkBuffer vb = m.vertexBuffer;
    VkDeviceSize offset = 0;
    vkCmdBindVertexBuffers(m_commandBuffer, 0, 1, &vb, &offset);

    if (m.indexCount > 0) {
        vkCmdBindIndexBuffer(m_commandBuffer, m.indexBuffer, 0, VK_INDEX_TYPE_UINT32);
        vkCmdDrawIndexed(m_commandBuffer, m.indexCount, 1, 0, 0, 0);
    } else {
        vkCmdDraw(m_commandBuffer, m.vertexCount, 1, 0, 0);
    }
}

// ==================================================================
// Resize
// ==================================================================
void VulkanRenderer::OnResize(u32 width, u32 height) {
    if (width == 0 || height == 0) return;
    vkDeviceWaitIdle(m_device);

    DestroySwapChain();
    CreateSwapChain(width, height);
    CreateFramebuffers();

    LOG_INFO("Vulkan resize: {}x{}", m_width, m_height);
}

} // namespace painkiller
