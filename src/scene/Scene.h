#pragma once

#include "Component.h"
#include "Entity.h"
#include "Transform.h"
#include "Camera.h"
#include "RenderTypes.h"
#include "Renderer.h"
#include "components/TransformComponent.h"
#include "components/MeshComponent.h"
#include "components/CameraComponent.h"
#include "components/LightComponent.h"
#include "components/MaterialComponent.h"

#include <vector>
#include <string>
#include <unordered_map>

namespace nebula {

// ?? Scene: Container for entities and their components ??
class Scene {
public:
    Scene() = default;
    ~Scene() = default;

    // ?? Entity Management ??
    EntityID CreateEntity(const std::string& name = "Entity");
    void DestroyEntity(EntityID entity);
    Entity* GetEntity(EntityID entity);
    const std::vector<Entity>& GetEntities() const { return m_entities; }
    
    // ?? Component Management ??
    template<typename T>
    T* AddComponent(EntityID entity, const T& component);
    
    template<typename T>
    T* GetComponent(EntityID entity);
    
    template<typename T>
    void RemoveComponent(EntityID entity);

    // ?? Transform ??
    TransformComponent* GetTransform(EntityID entity);
    Mat4 GetWorldMatrix(EntityID entity);

    // ?? Camera ??
    Camera* GetActiveCamera() { return &m_activeCamera; }
    CameraComponent* GetCameraComponent(EntityID entity);
    
    // ?? Rendering ??
    void RenderScene(Renderer* renderer, f32 deltaTime);
    
    // ?? Lights ??
    std::vector<EntityID> GetLightEntities() const { return m_lightEntities; }

private:
    std::vector<Entity> m_entities;
    
    // Component storage (flat arrays for cache efficiency)
    std::vector<TransformComponent> m_transformComponents;
    std::vector<MeshComponent>      m_meshComponents;
    std::vector<CameraComponent>    m_cameraComponents;
    std::vector<LightComponent>     m_lightComponents;
    std::vector<MaterialComponent>  m_materialComponents;
    
    // Entity -> component index mapping
    std::unordered_map<EntityID, u32> m_entityIndex;
    std::unordered_map<EntityID, u32> m_transformIndex;
    std::unordered_map<EntityID, u32> m_meshIndex;
    std::unordered_map<EntityID, u32> m_cameraIndex;
    std::unordered_map<EntityID, u32> m_lightIndex;
    std::unordered_map<EntityID, u32> m_materialIndex;
    
    std::vector<EntityID> m_lightEntities;
    
    Camera m_activeCamera;
    EntityID m_nextEntityID = 0;
};

} // namespace nebula

