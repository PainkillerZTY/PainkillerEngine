#pragma once

#include "Engine.h"

namespace nebula {

// ?? Demo Application ??
class DemoApp {
public:
    DemoApp() = default;
    ~DemoApp() = default;
    
    bool Initialize(Engine* engine);
    void Update(Engine* engine, f32 deltaTime);
    void Render(Engine* engine, f32 deltaTime);
    void Shutdown(Engine* engine);

private:
    // Scene objects
    EntityID m_cubeEntity   = kInvalidEntity;
    EntityID m_groundEntity = kInvalidEntity;
    EntityID m_lightEntity  = kInvalidEntity;
    EntityID m_cameraPivot  = kInvalidEntity;
    
    // Shaders
    ShaderHandle m_vs = kInvalidHandle;
    ShaderHandle m_fs = kInvalidHandle;
    PipelineHandle m_pipeline = kInvalidHandle;
    
    // Meshes
    MeshHandle m_cubeMesh   = kInvalidHandle;
    MeshHandle m_planeMesh  = kInvalidHandle;
    MeshHandle m_sphereMesh = kInvalidHandle;
    
    // Pipeline state
    bool m_pipelineCreated = false;
    
    f32 m_rotationAngle = 0.0f;
    f32 m_cameraDistance = 5.0f;
    f32 m_cameraAngle = 0.0f;
    f32 m_cameraHeight = 2.0f;
};

} // namespace nebula
