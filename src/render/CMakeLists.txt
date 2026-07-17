add_library(painkiller_render OBJECT
    RenderTypes.h
    Renderer.h Renderer.cpp
    OpenGLRenderer.h OpenGLRenderer.cpp
    DirectX11Renderer.h DirectX11Renderer.cpp
    VulkanRenderer.h VulkanRenderer.cpp
    Shader.h Shader.cpp
    Buffer.h Buffer.cpp
    Texture.h Texture.cpp
    PipelineState.h PipelineState.cpp
    Mesh.h Mesh.cpp
)
target_include_directories(painkiller_render PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(painkiller_render PUBLIC painkiller_core painkiller_math glm)
if(WIN32)
    target_link_libraries(painkiller_render PUBLIC opengl32 d3d11 dxgi d3dcompiler)
endif()
if(TARGET glfw)
    target_link_libraries(painkiller_render PUBLIC glfw)
endif()
