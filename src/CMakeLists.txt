 # ── Engine Core Libraries ──
 add_subdirectory(core)
 add_subdirectory(math)
 add_subdirectory(render)
 add_subdirectory(scene)
 add_subdirectory(resource)
 add_subdirectory(engine)
 
 # ── Engine Combined Library ──
 add_library(nebula_engine STATIC
     $<TARGET_OBJECTS:nebula_core>
     $<TARGET_OBJECTS:nebula_math>
     $<TARGET_OBJECTS:nebula_render>
     $<TARGET_OBJECTS:nebula_scene>
     $<TARGET_OBJECTS:nebula_resource>
     $<TARGET_OBJECTS:nebula_engine_core>
 )
 
 target_link_libraries(nebula_engine
     PUBLIC
         nebula_core
         nebula_math
         nebula_render
         nebula_scene
         nebula_resource
         nebula_engine_core
         glm
         stb
 )
 
 if(TARGET glfw)
     target_link_libraries(nebula_engine PUBLIC glfw)
 endif()
