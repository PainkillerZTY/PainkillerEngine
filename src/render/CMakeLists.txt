add_library(nebula_render OBJECT
    RenderTypes.h
    Renderer.h Renderer.cpp
    OpenGLRenderer.h OpenGLRenderer.cpp
    Shader.h Shader.cpp
    Buffer.h Buffer.cpp
    Texture.h Texture.cpp
    PipelineState.h PipelineState.cpp
    Mesh.h Mesh.cpp
)
target_include_directories(nebula_render PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(nebula_render PUBLIC nebula_core nebula_math glm)
if(WIN32)
    target_link_libraries(nebula_render PUBLIC opengl32)
endif()
if(TARGET glfw)
    target_link_libraries(nebula_render PUBLIC glfw)
endif()
