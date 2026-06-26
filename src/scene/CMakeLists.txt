add_library(nebula_scene OBJECT
    Scene.h Scene.cpp
    Entity.h Entity.cpp
    Component.h
    components/TransformComponent.h
    components/MeshComponent.h
    components/CameraComponent.h
    components/LightComponent.h
    components/MaterialComponent.h
    Camera.h Camera.cpp
)
target_include_directories(nebula_scene PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(nebula_scene PUBLIC nebula_core nebula_math nebula_render glm)
