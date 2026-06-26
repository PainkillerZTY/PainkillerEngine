#include "Scene.h"
#include "Logger.h"
#include <algorithm>

namespace nebula {

EntityID Scene::CreateEntity(const std::string& name) {
    EntityID id = m_nextEntityID++;
    m_entities.emplace_back(id, name);
    m_entityIndex[id] = (u32)(m_entities.size() - 1);
    
    TransformComponent tc;
    m_transformComponents.push_back(tc);
    m_transformIndex[id] = (u32)(m_transformComponents.size() - 1);
    
    LOG_DEBUG("Created entity: {} (ID: {})", name, id);
    return id;
}

void Scene::DestroyEntity(EntityID entity) {
    auto it = m_entityIndex.find(entity);
    if (it == m_entityIndex.end()) return;
    
    m_transformIndex.erase(entity);
    m_meshIndex.erase(entity);
    m_cameraIndex.erase(entity);
    m_lightIndex.erase(entity);
    m_materialIndex.erase(entity);
    
    auto lit = std::find(m_lightEntities.begin(), m_lightEntities.end(), entity);
    if (lit != m_lightEntities.end()) m_lightEntities.erase(lit);
    
    m_entityIndex.erase(it);
    LOG_DEBUG("Destroyed entity: {}", entity);
}

Entity* Scene::GetEntity(EntityID entity) {
    auto it = m_entityIndex.find(entity);
    return it != m_entityIndex.end() ? &m_entities[it->second] : nullptr;
}

template<>
TransformComponent* Scene::AddComponent<TransformComponent>(EntityID entity, const TransformComponent& component) {
    m_transformComponents.push_back(component);
    m_transformIndex[entity] = (u32)(m_transformComponents.size() - 1);
    return &m_transformComponents.back();
}
template<>
MeshComponent* Scene::AddComponent<MeshComponent>(EntityID entity, const MeshComponent& component) {
    m_meshComponents.push_back(component);
    m_meshIndex[entity] = (u32)(m_meshComponents.size() - 1);
    auto* e = GetEntity(entity);
    if (e) e->AddComponentFlag(ComponentType::Mesh);
    return &m_meshComponents.back();
}
template<>
CameraComponent* Scene::AddComponent<CameraComponent>(EntityID entity, const CameraComponent& component) {
    m_cameraComponents.push_back(component);
    m_cameraIndex[entity] = (u32)(m_cameraComponents.size() - 1);
    auto* e = GetEntity(entity);
    if (e) e->AddComponentFlag(ComponentType::Camera);
    return &m_cameraComponents.back();
}
template<>
LightComponent* Scene::AddComponent<LightComponent>(EntityID entity, const LightComponent& component) {
    m_lightComponents.push_back(component);
    m_lightIndex[entity] = (u32)(m_lightComponents.size() - 1);
    m_lightEntities.push_back(entity);
    auto* e = GetEntity(entity);
    if (e) e->AddComponentFlag(ComponentType::Light);
    return &m_lightComponents.back();
}
template<>
MaterialComponent* Scene::AddComponent<MaterialComponent>(EntityID entity, const MaterialComponent& component) {
    m_materialComponents.push_back(component);
    m_materialIndex[entity] = (u32)(m_materialComponents.size() - 1);
    auto* e = GetEntity(entity);
    if (e) e->AddComponentFlag(ComponentType::Material);
    return &m_materialComponents.back();
}

template<>
TransformComponent* Scene::GetComponent<TransformComponent>(EntityID entity) {
    auto it = m_transformIndex.find(entity);
    return it != m_transformIndex.end() ? &m_transformComponents[it->second] : nullptr;
}
template<>
MeshComponent* Scene::GetComponent<MeshComponent>(EntityID entity) {
    auto it = m_meshIndex.find(entity);
    return it != m_meshIndex.end() ? &m_meshComponents[it->second] : nullptr;
}
template<>
CameraComponent* Scene::GetComponent<CameraComponent>(EntityID entity) {
    auto it = m_cameraIndex.find(entity);
    return it != m_cameraIndex.end() ? &m_cameraComponents[it->second] : nullptr;
}
template<>
LightComponent* Scene::GetComponent<LightComponent>(EntityID entity) {
    auto it = m_lightIndex.find(entity);
    return it != m_lightIndex.end() ? &m_lightComponents[it->second] : nullptr;
}
template<>
MaterialComponent* Scene::GetComponent<MaterialComponent>(EntityID entity) {
    auto it = m_materialIndex.find(entity);
    return it != m_materialIndex.end() ? &m_materialComponents[it->second] : nullptr;
}

TransformComponent* Scene::GetTransform(EntityID entity) {
    return GetComponent<TransformComponent>(entity);
}

Mat4 Scene::GetWorldMatrix(EntityID entity) {
    auto* tc = GetTransform(entity);
    return tc ? tc->ToMat4() : Mat4(1.0f);
}

CameraComponent* Scene::GetCameraComponent(EntityID entity) {
    return GetComponent<CameraComponent>(entity);
}

void Scene::RenderScene(Renderer* renderer, f32 deltaTime) {
    (void)deltaTime;
    if (!renderer) return;
    
    for (auto it = m_meshIndex.begin(); it != m_meshIndex.end(); ++it) {
        EntityID entityId = it->first;
        u32 idx = it->second;
        auto* meshComp = &m_meshComponents[idx];
        if (meshComp->meshHandle == kInvalidHandle) continue;
        
        Mat4 worldMatrix = GetWorldMatrix(entityId);
        
        renderer->SetUniformMat4("u_Model", worldMatrix);
        renderer->SetUniformMat4("u_View", m_activeCamera.GetViewMatrix());
        renderer->SetUniformMat4("u_Projection", m_activeCamera.GetProjectionMatrix());
        renderer->SetUniformVec3("u_CameraPos", m_activeCamera.GetPosition());
        
        if (!m_lightEntities.empty()) {
            auto* light = GetComponent<LightComponent>(m_lightEntities[0]);
            if (light) {
                auto* lightTransform = GetTransform(m_lightEntities[0]);
                Vec3 lightDir = lightTransform ? lightTransform->position : Vec3(0.5f, -1.0f, 0.3f);
                renderer->SetUniformVec3("u_LightDir", glm::normalize(lightDir));
                renderer->SetUniformVec3("u_LightColor", light->color * light->intensity);
            }
        }
        
        renderer->DrawMesh(meshComp->meshHandle);
    }
}

} // namespace nebula
