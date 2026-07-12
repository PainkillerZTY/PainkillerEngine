add_library(painkiller_engine_core OBJECT
    Engine.h Engine.cpp
    Window.h Window.cpp
    Timer.h Timer.cpp
    Input.h Input.cpp
)
target_include_directories(painkiller_engine_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(painkiller_engine_core PUBLIC painkiller_core painkiller_math painkiller_render painkiller_scene painkiller_resource glm)
if(TARGET glfw)
    target_link_libraries(painkiller_engine_core PUBLIC glfw)
endif()
