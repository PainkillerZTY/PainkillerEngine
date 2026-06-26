add_library(nebula_resource OBJECT
    ResourceManager.h ResourceManager.cpp
    ModelLoader.h ModelLoader.cpp
    TextureLoader.h TextureLoader.cpp
)
target_include_directories(nebula_resource PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(nebula_resource PUBLIC nebula_core nebula_render stb)
