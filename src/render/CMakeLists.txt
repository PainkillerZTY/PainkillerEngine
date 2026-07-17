add_library(painkiller_render OBJECT
    RenderTypes.h
    Renderer.h Renderer.cpp
    OpenGLRenderer.h OpenGLRenderer.cpp
    Shader.h Shader.cpp
    Buffer.h Buffer.cpp
    Texture.h Texture.cpp
    PipelineState.h PipelineState.cpp
    Mesh.h Mesh.cpp
)
target_include_directories(painkiller_render PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(painkiller_render PUBLIC painkiller_core painkiller_math glm)
if(WIN32)
    target_link_libraries(painkiller_render PUBLIC opengl32)
endif()
if(TARGET glfw)
    target_link_libraries(painkiller_render PUBLIC glfw)
endif()

# ---- DirectX 11 backend (optional) ----
if(PAINKILLER_ENABLE_D3D11)
    target_sources(painkiller_render PRIVATE
        DirectX11Renderer.h DirectX11Renderer.cpp
    )
    target_include_directories(painkiller_render PRIVATE ${PAINKILLER_D3D11_INCLUDE_DIR})
    target_link_directories(painkiller_render PRIVATE ${PAINKILLER_D3D11_LIB_DIR})
    target_link_libraries(painkiller_render PRIVATE d3d11 dxgi d3dcompiler)
    target_compile_definitions(painkiller_render PRIVATE PAINKILLER_HAS_D3D11)
endif()

# ---- Vulkan backend (optional) ----
if(PAINKILLER_ENABLE_VULKAN)
    target_sources(painkiller_render PRIVATE
        VulkanRenderer.h VulkanRenderer.cpp
    )
    target_include_directories(painkiller_render PRIVATE ${PAINKILLER_VULKAN_INCLUDE_DIR})
    target_link_directories(painkiller_render PRIVATE ${PAINKILLER_VULKAN_LIB_DIR})
    target_link_libraries(painkiller_render PRIVATE vulkan-1)
    target_compile_definitions(painkiller_render PRIVATE PAINKILLER_HAS_VULKAN)
endif()
