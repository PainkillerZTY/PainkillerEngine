 add_library(painkiller_resource OBJECT
     ResourceManager.h ResourceManager.cpp
     ModelLoader.h ModelLoader.cpp
     TextureLoader.h TextureLoader.cpp
     stb_impl.cpp
 )
target_include_directories(painkiller_resource PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(painkiller_resource PUBLIC painkiller_core painkiller_render stb)
