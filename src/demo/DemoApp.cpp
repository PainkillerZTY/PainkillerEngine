#include "DemoApp.h"
#include "Logger.h"

namespace nebula {

// ?? Embedded Shader Sources ??

// Vertex shader (GLSL 330 core)
static const char* kVertexShaderSrc = R"(
    #version 330 core
    
    layout(location = 0) in vec3 a_Position;
    layout(location = 1) in vec3 a_Normal;
    layout(location = 2) in vec2 a_TexCoord;
    
    uniform mat4 u_Model;
    uniform mat4 u_View;
    uniform mat4 u_Projection;
    uniform vec3 u_CameraPos;
    uniform vec3 u_LightDir;
    uniform vec3 u_LightColor;
    
    out vec3 v_Normal;
    out vec3 v_FragPos;
    out vec3 v_ViewDir;
    out vec2 v_TexCoord;
    
    void main() {
        vec4 worldPos = u_Model * vec4(a_Position, 1.0);
        gl_Position = u_Projection * u_View * worldPos;
        
        v_Normal   = normalize(mat3(transpose(inverse(u_Model))) * a_Normal);
        v_FragPos  = worldPos.xyz;
        v_ViewDir  = normalize(u_CameraPos - worldPos.xyz);
        v_TexCoord = a_TexCoord;
    }
)";

// Fragment shader (GLSL 330 core)
static const char* kFragmentShaderSrc = R"(
    #version 330 core
    
    in vec3 v_Normal;
    in vec3 v_FragPos;
    in vec3 v_ViewDir;
    in vec2 v_TexCoord;
    
    uniform vec3 u_LightDir;
    uniform vec3 u_LightColor;
    uniform float u_Time;
    
    out vec4 FragColor;
    
    void main() {
        vec3 normal = normalize(v_Normal);
        vec3 lightDir = normalize(u_LightDir);
        
        // Ambient
        vec3 ambient = vec3(0.15, 0.15, 0.2);
        
        // Diffuse
        float diff = max(dot(normal, -lightDir), 0.0);
        vec3 diffuse = diff * u_LightColor;
        
        // Specular (Blinn-Phong)
        vec3 viewDir = normalize(v_ViewDir);
        vec3 halfDir = normalize(-lightDir + viewDir);
        float spec = pow(max(dot(normal, halfDir), 0.0), 32.0);
        vec3 specular = spec * u_LightColor * 0.5;
        
        // Color (rainbow cycling for fun, or use albedo)
        vec3 color = vec3(0.3, 0.5, 0.9);
        
        // Checker pattern using world position
        vec3 worldPos = v_FragPos;
        float checker = step(0.5, fract(worldPos.x * 2.0) * fract(worldPos.z * 2.0));
        color = mix(color, color * 1.3, step(0.5, fract(worldPos.x * 2.0) * fract(worldPos.z * 2.0)));
        color = mix(color, color * 0.7, step(0.5, fract(worldPos.x * 2.0 + 1.0) * fract(worldPos.z * 2.0 + 1.0)));
        
        vec3 result = (ambient + diffuse + specular) * color;
        FragColor = vec4(result, 1.0);
    }
)";

// ?? Initialize Demo ??
bool DemoApp::Initialize(Engine* engine) {
    LOG_INFO("DemoApp initializing...");
    auto* renderer = engine->GetRenderer();
    auto* resourceManager = engine->GetResourceManager();
    auto* scene = engine->GetScene();
    
    // 1. Create shaders
    ShaderDesc vsDesc;
    vsDesc.stage = ShaderStage::Vertex;
    vsDesc.source = kVertexShaderSrc;
    vsDesc.entryPoint = "main";
    m_vs = renderer->CreateShader(vsDesc);
    
    ShaderDesc fsDesc;
    fsDesc.stage = ShaderStage::Fragment;
    fsDesc.source = kFragmentShaderSrc;
    fsDesc.entryPoint = "main";
    m_fs = renderer->CreateShader(fsDesc);
    
    if (m_vs == kInvalidHandle || m_fs == kInvalidHandle) {
        LOG_ERROR("Failed to create demo shaders");
        return false;
    }
    
    // 2. Create pipeline state
    PipelineStateDesc pipelineDesc;
    pipelineDesc.vertexShader = &vsDesc;
    pipelineDesc.fragmentShader = &fsDesc;
    pipelineDesc.topology = PrimitiveTopology::TriangleList;
    pipelineDesc.cullMode = CullMode::Back;
    pipelineDesc.fillMode = FillMode::Solid;
    pipelineDesc.depthTestEnabled = true;
    pipelineDesc.depthWriteEnabled = true;
    pipelineDesc.depthFunc = CompareFunction::Less;
    
    // Vertex layout: position(3) + normal(3) + texcoord(2)
    pipelineDesc.vertexLayout = {
        { "POSITION", Format::R32G32B32_FLOAT, 0, 0 },
        { "NORMAL",   Format::R32G32B32_FLOAT, 12, 0 },
        { "TEXCOORD", Format::R32G32_FLOAT, 24, 0 },
    };
    
    // Shader sources need to be alive
    m_pipeline = renderer->CreatePipelineState(pipelineDesc);
    if (m_pipeline == kInvalidHandle) {
        LOG_ERROR("Failed to create pipeline");
        return false;
    }
    
    // 3. Create meshes
    m_cubeMesh   = resourceManager->LoadMesh("cube",   MeshData::CreateCube(1.0f));
    m_planeMesh  = resourceManager->LoadMesh("plane",  MeshData::CreatePlane(10.0f, 10.0f));
    m_sphereMesh = resourceManager->LoadMesh("sphere", MeshData::CreateSphere(0.5f, 24, 16));
    
    // 4. Create entities
    // ?? Light ??
    m_lightEntity = scene->CreateEntity("DirectionalLight");
    auto* lightTransform = scene->GetTransform(m_lightEntity);
    lightTransform->position = Vec3(5.0f, 8.0f, 5.0f);
    LightComponent light;
    light.type = LightType::Directional;
    light.color = Vec3(1.0f, 0.95f, 0.85f);
    light.intensity = 1.5f;
    scene->AddComponent<LightComponent>(m_lightEntity, light);
    
    // ?? Camera Pivot (for orbit) ??
    m_cameraPivot = scene->CreateEntity("CameraPivot");
    auto* pivotTransform = scene->GetTransform(m_cameraPivot);
    pivotTransform->position = Vec3(0.0f, 0.0f, 0.0f);
    
    // ?? Ground ??
    m_groundEntity = scene->CreateEntity("Ground");
    auto* groundTransform = scene->GetTransform(m_groundEntity);
    groundTransform->position = Vec3(0.0f, -0.5f, 0.0f);
    groundTransform->scale = Vec3(1.0f, 1.0f, 1.0f);
    MeshComponent groundMesh;
    groundMesh.meshHandle = m_planeMesh;
    scene->AddComponent<MeshComponent>(m_groundEntity, groundMesh);
    
    // ?? Main Cube ??
    m_cubeEntity = scene->CreateEntity("MainCube");
    auto* cubeTransform = scene->GetTransform(m_cubeEntity);
    cubeTransform->position = Vec3(0.0f, 0.5f, 0.0f);
    MeshComponent cubeMesh;
    cubeMesh.meshHandle = m_cubeMesh;
    scene->AddComponent<MeshComponent>(m_cubeEntity, cubeMesh);
    
    // ?? Orbiting Sphere ??
    EntityID sphereEntity = scene->CreateEntity("Orbiter");
    auto* sphereTransform = scene->GetTransform(sphereEntity);
    sphereTransform->position = Vec3(2.0f, 0.5f, 0.0f);
    sphereTransform->scale = Vec3(0.5f, 0.5f, 0.5f);
    MeshComponent sphereMesh;
    sphereMesh.meshHandle = m_sphereMesh;
    scene->AddComponent<MeshComponent>(sphereEntity, sphereMesh);
    
    // ?? Bind pipeline ??
    renderer->BindPipelineState(m_pipeline);
    
    m_pipelineCreated = true;
    LOG_INFO("DemoApp initialized successfully");
    return true;
}

// ?? Update ??
void DemoApp::Update(Engine* engine, f32 deltaTime) {
    auto* scene = engine->GetScene();
    auto* input = engine->GetInput();
    
    // Rotate the cube
    m_rotationAngle += deltaTime * 30.0f; // 30 degrees per second
    
    auto* cubeTransform = scene->GetTransform(m_cubeEntity);
    if (cubeTransform) {
        cubeTransform->eulerRotation.y = m_rotationAngle;
    }
    
    // Orbit camera controls
    f32 orbitSpeed = 20.0f;
    f32 zoomSpeed = 2.0f;
    
    if (input->IsKeyDown(VK_LEFT))  m_cameraAngle -= orbitSpeed * deltaTime;
    if (input->IsKeyDown(VK_RIGHT)) m_cameraAngle += orbitSpeed * deltaTime;
    if (input->IsKeyDown(VK_UP))    m_cameraHeight += 1.0f * deltaTime;
    if (input->IsKeyDown(VK_DOWN))  m_cameraHeight -= 1.0f * deltaTime;
    
    // Mouse orbit (if left mouse button is held)
    if (input->IsMouseDown(0)) {
        m_cameraAngle -= input->GetMouseDeltaX() * 0.5f * deltaTime;
        m_cameraHeight += input->GetMouseDeltaY() * 0.3f * deltaTime;
    }
    
    // Scroll to zoom
    m_cameraDistance -= input->GetScrollDelta() * 0.1f;
    m_cameraDistance = glm::clamp(m_cameraDistance, 2.0f, 20.0f);
    m_cameraHeight = glm::clamp(m_cameraHeight, 0.5f, 10.0f);
    
    // Update camera position
    Vec3 camPos;
    camPos.x = m_cameraDistance * sinf(m_cameraAngle);
    camPos.z = m_cameraDistance * cosf(m_cameraAngle);
    camPos.y = m_cameraHeight;
    
    scene->GetActiveCamera()->SetPosition(camPos);
    scene->GetActiveCamera()->LookAt(camPos, Vec3(0.0f, 0.5f, 0.0f));
    
    // Exit on Escape
    if (input->IsKeyPressed(VK_ESCAPE)) {
        engine->Stop();
    }
}

// ?? Render ??
void DemoApp::Render(Engine* engine, f32 deltaTime) {
    auto* renderer = engine->GetRenderer();
    auto* scene = engine->GetScene();
    
    if (!m_pipelineCreated) return;
    
    // Bind pipeline and update shared uniforms
    renderer->BindPipelineState(m_pipeline);
    
    // Uniforms are set per-entity in Scene::RenderScene
}

// ?? Shutdown ??
void DemoApp::Shutdown(Engine* engine) {
    LOG_INFO("DemoApp shutting down...");
    auto* renderer = engine->GetRenderer();
    
    if (m_pipeline != kInvalidHandle) {
        renderer->DestroyPipelineState(m_pipeline);
    }
    if (m_vs != kInvalidHandle) {
        renderer->DestroyShader(m_vs);
    }
    if (m_fs != kInvalidHandle) {
        renderer->DestroyShader(m_fs);
    }
}

} // namespace nebula
