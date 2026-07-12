# Painkiller Engine Core Libraries
add_subdirectory(core)
add_subdirectory(math)
add_subdirectory(render)
add_subdirectory(scene)
add_subdirectory(resource)
add_subdirectory(engine)

# Static library containing all engine object libraries
add_library(painkiller_engine STATIC
    $<TARGET_OBJECTS:painkiller_core>
    $<TARGET_OBJECTS:painkiller_math>
    $<TARGET_OBJECTS:painkiller_render>
    $<TARGET_OBJECTS:painkiller_scene>
    $<TARGET_OBJECTS:painkiller_resource>
    $<TARGET_OBJECTS:painkiller_engine_core>)

 target_link_libraries(painkiller_engine PUBLIC
     painkiller_core
     painkiller_math
     painkiller_render
     painkiller_scene
     painkiller_resource
     painkiller_engine_core)
 if(TARGET glfw)
     target_link_libraries(painkiller_engine PUBLIC glfw)
 else()
     target_link_libraries(painkiller_engine PUBLIC glfw3)
 endif()
