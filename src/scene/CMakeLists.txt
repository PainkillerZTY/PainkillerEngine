add_library(painkiller_scene OBJECT
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
target_include_directories(painkiller_scene PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(painkiller_scene PUBLIC painkiller_core painkiller_math painkiller_render glm)
