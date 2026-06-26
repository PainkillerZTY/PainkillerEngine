add_library(nebula_engine_core OBJECT
    Engine.h Engine.cpp
    Window.h Window.cpp
    Timer.h Timer.cpp
    Input.h Input.cpp
)
target_include_directories(nebula_engine_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(nebula_engine_core PUBLIC nebula_core nebula_math nebula_render nebula_scene nebula_resource glm)
if(TARGET glfw)
    target_link_libraries(nebula_engine_core PUBLIC glfw)
endif()
