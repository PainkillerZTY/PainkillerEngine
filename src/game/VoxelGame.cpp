#include "VoxelGame.h"
#include "Logger.h"
#include "Math.h"
#include "BlockRaycast.h"
#include <cstdlib>
#include <algorithm>
#include <cstdio>

namespace nebula {

// ============================================================
// Embedded World Shaders
// ============================================================
static const char* kWorldVertexSrc = R"(
    #version 330 core
    layout(location = 0) in vec3 a_Position;
    layout(location = 1) in vec3 a_Normal;
    layout(location = 2) in vec2 a_TexCoord;

    uniform mat4 u_View;
    uniform mat4 u_Projection;
    uniform vec3 u_CameraPos;

    out vec3 v_Normal;
    out vec3 v_FragPos;
    out vec3 v_ViewDir;
    out vec2 v_TexCoord;

    void main() {
        vec4 worldPos = vec4(a_Position, 1.0);
        gl_Position = u_Projection * u_View * worldPos;
        v_Normal   = a_Normal;
        v_FragPos  = a_Position;
        v_ViewDir  = normalize(u_CameraPos - a_Position);
        v_TexCoord = a_TexCoord;
    }
)";

static const char* kWorldFragmentSrc = R"(
    #version 330 core
    in vec3 v_Normal;
    in vec3 v_FragPos;
    in vec3 v_ViewDir;
    in vec2 v_TexCoord;

    uniform vec3 u_LightDir;
    uniform vec3 u_LightColor;
    uniform float u_Time;

    out vec4 FragColor;

    int getBlockType() { return int(v_TexCoord.x * 255.0 + 0.5); }
    int getFace()      { return int(v_TexCoord.y * 6.0 + 0.5); }

    vec3 getBlockColor(int type, int face) {
        if (type == 1) {
            if (face == 2) return vec3(0.29, 0.65, 0.14);
            if (face == 3) return vec3(0.45, 0.32, 0.18);
            return vec3(0.27, 0.50, 0.12);
        }
        if (type == 2) return vec3(0.45, 0.32, 0.18);
        if (type == 3) {
            float n = fract(v_FragPos.x * 0.1 + v_FragPos.z * 0.1 + v_FragPos.y * 0.1);
            return vec3(0.45, 0.45, 0.45) + vec3(n * 0.1);
        }
        if (type == 4) return vec3(0.40, 0.40, 0.40);
        if (type == 5) return vec3(0.42, 0.29, 0.13);
        if (type == 6) {
            if (face == 2 || face == 3) return vec3(0.55, 0.37, 0.16);
            return vec3(0.42, 0.29, 0.13);
        }
        if (type == 7) {
            float f = 0.15 + 0.05 * sin(v_FragPos.x * 3.0 + v_FragPos.z * 2.0);
            return vec3(f, 0.50, f * 0.5);
        }
        if (type == 8) return vec3(0.52, 0.36, 0.18);
        if (type == 9) return vec3(0.76, 0.70, 0.50);
        if (type == 10) return vec3(0.20, 0.40, 0.80);
        if (type == 11) return vec3(0.95, 0.95, 0.98);
        if (type == 12) return vec3(0.20, 0.20, 0.20);
        return vec3(1.0, 0.0, 1.0);
    }

    void main() {
        vec3 normal = normalize(v_Normal);
        vec3 lightDir = normalize(u_LightDir);
        vec3 ambient = vec3(0.20, 0.22, 0.25);
        float diff = max(dot(normal, -lightDir), 0.0);
        vec3 diffuse = diff * u_LightColor;
        vec3 viewDir = normalize(v_ViewDir);
        vec3 halfDir = normalize(-lightDir + viewDir);
        float spec = pow(max(dot(normal, halfDir), 0.0), 8.0);
        vec3 specular = spec * u_LightColor * 0.1;
        vec3 color = getBlockColor(getBlockType(), getFace());
        vec3 result = (ambient + diffuse + specular) * color;
        // Distance fog
        float dist = length(v_FragPos);
        float fog = clamp((dist - 40.0) / 60.0, 0.0, 0.85);
        vec3 fogColor = vec3(0.6, 0.7, 0.9);
        FragColor.rgb = mix(result, fogColor, fog);
        FragColor.a = 1.0;
    }
)";

// ============================================================
// VoxelGame Implementation
// ============================================================
VoxelGame::VoxelGame() {}
VoxelGame::~VoxelGame() { Shutdown(nullptr); }

bool VoxelGame::Initialize(Engine* engine) {
    LOG_INFO("VoxelGame initializing...");
    auto* renderer = engine->GetRenderer();
    auto* scene = engine->GetScene();

    // 1. Create world
    m_world = new World(renderer);
    m_world->Initialize(m_seed);

    // 2. Setup shaders
    SetupShadersAndPipeline(renderer);

    // 3. Init player
    m_player.Initialize(scene->GetActiveCamera());

    // 4. Particle system
    m_particleSystem.Initialize(renderer);

    // 5. Sound
    m_soundManager.Initialize();

    // 6. Load initial chunks
    UpdateChunks(engine);

    // 7. Crosshair mesh (centered at origin, translated in RenderUI)
    {
        MeshData ch;
        ch.vertexStride = 8 * sizeof(f32);
        f32 s = 0.5f;
        f32 len = 4.0f;
        f32 verts[] = {
            -len, -s, 0,  0,0,1,  0,0,
             len, -s, 0,  0,0,1,  1,0,
             len,  s, 0,  0,0,1,  1,1,
            -len,  s, 0,  0,0,1,  0,1,
             -s, -len, 0, 0,0,1,  0,0,
              s, -len, 0, 0,0,1,  1,0,
              s,  len, 0, 0,0,1,  1,1,
             -s,  len, 0, 0,0,1,  0,1,
        };
        ch.vertices.assign(verts, verts + 64);
        ch.vertexCount = 8;
        u32 idx[] = { 0,1,2, 0,2,3, 4,5,6, 4,6,7 };
        ch.indices.assign(idx, idx + 12);
        ch.indexCount = 12;
        m_crosshairMesh = renderer->CreateMesh(ch);
    }

    m_initialized = true;
    LOG_INFO("VoxelGame initialized");
    return true;
}

void VoxelGame::SetupShadersAndPipeline(Renderer* renderer) {
    m_worldVSDesc.stage = ShaderStage::Vertex;
    m_worldVSDesc.source = kWorldVertexSrc;
    m_worldVS = renderer->CreateShader(m_worldVSDesc);

    m_worldFSDesc.stage = ShaderStage::Fragment;
    m_worldFSDesc.source = kWorldFragmentSrc;
    m_worldFS = renderer->CreateShader(m_worldFSDesc);

    if (m_worldVS == kInvalidHandle || m_worldFS == kInvalidHandle) {
        LOG_ERROR("Failed to create world shaders");
        return;
    }

    PipelineStateDesc pd;
    pd.vertexShader = &m_worldVSDesc;
    pd.fragmentShader = &m_worldFSDesc;
    pd.topology = PrimitiveTopology::TriangleList;
    pd.cullMode = CullMode::Back;
    pd.fillMode = FillMode::Solid;
    pd.depthTestEnabled = true;
    pd.depthWriteEnabled = true;
    pd.depthFunc = CompareFunction::Less;
    pd.vertexLayout = {
        {"POSITION", Format::R32G32B32_FLOAT, 0, 0},
        {"NORMAL",   Format::R32G32B32_FLOAT, 12, 0},
        {"TEXCOORD", Format::R32G32_FLOAT, 24, 0},
    };
    m_worldPipeline = renderer->CreatePipelineState(pd);
    if (m_worldPipeline == kInvalidHandle) {
        LOG_ERROR("Failed to create world pipeline");
        return;
    }

    // UI shader
    const char* uiVS = R"(#version 330 core
        layout(location=0) in vec3 a_Position;
        uniform mat4 u_Projection;
        uniform mat4 u_Model;
        void main() {
            gl_Position = u_Projection * u_Model * vec4(a_Position, 1.0);
        }
    )";
    const char* uiFS = R"(#version 330 core
        uniform vec4 u_Color;
        out vec4 FragColor;
        void main() { FragColor = u_Color; }
    )";
    ShaderDesc uVS; uVS.stage = ShaderStage::Vertex; uVS.source = uiVS;
    ShaderDesc uFS; uFS.stage = ShaderStage::Fragment; uFS.source = uiFS;
    m_uiVS = renderer->CreateShader(uVS);
    m_uiFS = renderer->CreateShader(uFS);
    if (m_uiVS != kInvalidHandle && m_uiFS != kInvalidHandle) {
        PipelineStateDesc up;
        up.vertexShader = &uVS; up.fragmentShader = &uFS;
        up.topology = PrimitiveTopology::TriangleList;
        up.cullMode = CullMode::None;
        up.depthTestEnabled = false; up.depthWriteEnabled = false;
        up.blendMode = BlendMode::AlphaBlend;
        up.vertexLayout = { {"POSITION", Format::R32G32B32_FLOAT, 0, 0},
                            {"NORMAL", Format::R32G32B32_FLOAT, 12, 0},
                            {"TEXCOORD", Format::R32G32_FLOAT, 24, 0} };
        m_uiPipeline = renderer->CreatePipelineState(up);
    }
    m_pipelineCreated = true;
}

void VoxelGame::Update(Engine* engine, f32 deltaTime) {
    if (!m_initialized) return;
    m_gameTime += deltaTime;
    UpdateChunks(engine);
    HandleBlockInteraction(engine);
    m_player.Update(engine, deltaTime);
    m_particleSystem.Update(deltaTime);
    if (m_placeCooldown > 0.0f) m_placeCooldown -= deltaTime;
}

void VoxelGame::HandleBlockInteraction(Engine* engine) {
    auto* input = engine->GetInput();
    auto* camera = m_player.GetCamera();
    Vec3 eyePos = m_player.GetPosition() + Vec3(0.0f, 1.6f, 0.0f);
    Vec3 forward = camera->GetForward();
    m_raycastResult = BlockRaycast::Cast(eyePos, forward, 8.0f, m_world);

    if (input->IsMouseDown(0) && m_raycastResult.hit) {
        BlockType block = m_world->GetBlock(m_raycastResult.blockX,
                                             m_raycastResult.blockY,
                                             m_raycastResult.blockZ);
        if (block != BlockType::Air && block != BlockType::Water) {
            Vec3 wp((f32)m_raycastResult.blockX + 0.5f,
                    (f32)m_raycastResult.blockY + 0.5f,
                    (f32)m_raycastResult.blockZ + 0.5f);
            Vec3 bc = BlockInfo::GetFaceColor(block, BlockFace::Top);
            m_world->SetBlock(m_raycastResult.blockX, m_raycastResult.blockY,
                              m_raycastResult.blockZ, BlockType::Air);
            m_particleSystem.SpawnBreakParticles(wp, bc, 12);
            m_soundManager.PlayBlockBreak();
        }
    }

    if (input->IsMousePressed(1) && m_raycastResult.hit && m_placeCooldown <= 0.0f) {
        BlockType existing = m_world->GetBlock(m_raycastResult.placeX,
                                                m_raycastResult.placeY,
                                                m_raycastResult.placeZ);
        if (existing == BlockType::Air) {
            m_world->SetBlock(m_raycastResult.placeX, m_raycastResult.placeY,
                              m_raycastResult.placeZ, BlockType::Dirt);
            m_soundManager.PlayBlockPlace();
            m_placeCooldown = 0.3f;
        }
    }

    if (input->IsKeyPressed(VK_ESCAPE)) engine->Stop();
}

void VoxelGame::UpdateChunks(Engine* engine) {
    (void)engine;
    Vec3 p = m_player.GetPosition();
    m_world->UpdateChunksAround((i32)p.x, (i32)p.z, 6);
}

void VoxelGame::Render(Engine* engine, f32 deltaTime) {
    if (!m_initialized || !m_pipelineCreated) return;
    auto* renderer = engine->GetRenderer();
    auto* camera = m_player.GetCamera();
    auto* window = engine->GetWindow();
    f32 ww = (f32)window->GetWidth();
    f32 wh = (f32)window->GetHeight();
    RenderWorld(renderer, camera);
    m_particleSystem.Render(renderer);
    RenderUI(renderer, ww, wh);
}

void VoxelGame::RenderWorld(Renderer* renderer, Camera* camera) {
    renderer->BindPipelineState(m_worldPipeline);
    renderer->SetUniformMat4("u_View", camera->GetViewMatrix());
    renderer->SetUniformMat4("u_Projection", camera->GetProjectionMatrix());
    renderer->SetUniformVec3("u_CameraPos", camera->GetPosition());
    Vec3 ld = glm::normalize(Vec3(0.5f, -0.8f, 0.3f));
    renderer->SetUniformVec3("u_LightDir", ld);
    renderer->SetUniformVec3("u_LightColor", Vec3(1.0f, 0.95f, 0.85f) * 1.2f);
    renderer->SetUniformFloat("u_Time", m_gameTime);
    m_world->Render(renderer);
}

void VoxelGame::RenderUI(Renderer* renderer, f32 winW, f32 winH) {
    if (m_uiPipeline == kInvalidHandle) return;
    renderer->BindPipelineState(m_uiPipeline);
    Mat4 ortho = glm::ortho(0.0f, winW, 0.0f, winH, -1.0f, 1.0f);
    renderer->SetUniformMat4("u_Projection", ortho);
    renderer->SetUniformVec4("u_Color", Vec4(1.0f, 1.0f, 1.0f, 0.8f));

    if (m_crosshairMesh != kInvalidHandle) {
        Mat4 model = Translate(Mat4(1.0f), Vec3(winW * 0.5f, winH * 0.5f, 0.0f));
        renderer->SetUniformMat4("u_Model", model);
        renderer->DrawMesh(m_crosshairMesh);
    }
}

void VoxelGame::RenderCrosshair(Renderer* renderer) { (void)renderer; }
void VoxelGame::RenderDebugInfo(Engine* engine) { (void)engine; }

void VoxelGame::Shutdown(Engine* engine) {
    LOG_INFO("VoxelGame shutting down...");
    if (m_pipelineCreated && engine) {
        auto* r = engine->GetRenderer();
        if (r) {
            if (m_worldPipeline  != kInvalidHandle) r->DestroyPipelineState(m_worldPipeline);
            if (m_worldVS        != kInvalidHandle) r->DestroyShader(m_worldVS);
            if (m_worldFS        != kInvalidHandle) r->DestroyShader(m_worldFS);
            if (m_uiPipeline     != kInvalidHandle) r->DestroyPipelineState(m_uiPipeline);
            if (m_uiVS           != kInvalidHandle) r->DestroyShader(m_uiVS);
            if (m_uiFS           != kInvalidHandle) r->DestroyShader(m_uiFS);
            if (m_crosshairMesh  != kInvalidHandle) r->DestroyMesh(m_crosshairMesh);
        }
    }
    m_particleSystem.Shutdown();
    m_soundManager.Shutdown();
    delete m_world; m_world = nullptr;
    m_initialized = false;
}

} // namespace nebula
