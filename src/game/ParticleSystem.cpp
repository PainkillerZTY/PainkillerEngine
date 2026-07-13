#include "ParticleSystem.h"
#include "Logger.h"
#include "Math.h"
#include <cstdlib>
#include <algorithm>

namespace painkiller {

ParticleSystem::ParticleSystem() {}

ParticleSystem::~ParticleSystem() {
    Shutdown();
}

void ParticleSystem::Initialize(Renderer* renderer) {
    if (!renderer) return;

    // Create a small cube mesh for particles
    MeshData cube = MeshData::CreateCube(0.1f);

    // Create simple shaders for particles
    const char* vertSrc = R"(
        #version 330 core
        layout(location = 0) in vec3 a_Position;
        layout(location = 1) in vec3 a_Normal;
        layout(location = 2) in vec2 a_TexCoord;

        uniform mat4 u_View;
        uniform mat4 u_Projection;
        uniform vec3 u_ParticlePos;
        uniform float u_ParticleSize;

        void main() {
            vec3 pos = a_Position * u_ParticleSize + u_ParticlePos;
            gl_Position = u_Projection * u_View * vec4(pos, 1.0);
        }
    )";

    const char* fragSrc = R"(
        #version 330 core
        uniform vec3 u_ParticleColor;
        uniform float u_ParticleAlpha;
        out vec4 FragColor;

        void main() {
            FragColor = vec4(u_ParticleColor, u_ParticleAlpha);
        }
    )";

    ShaderDesc vsDesc;
    vsDesc.stage = ShaderStage::Vertex;
    vsDesc.source = vertSrc;
    m_vs = renderer->CreateShader(vsDesc);

    ShaderDesc fsDesc;
    fsDesc.stage = ShaderStage::Fragment;
    fsDesc.source = fragSrc;
    m_fs = renderer->CreateShader(fsDesc);

    if (m_vs == kInvalidHandle || m_fs == kInvalidHandle) {
        LOG_ERROR("Failed to create particle shaders");
        return;
    }

    // Create pipeline
    PipelineStateDesc pipelineDesc;
    pipelineDesc.vertexShader = &vsDesc;
    pipelineDesc.fragmentShader = &fsDesc;
    pipelineDesc.topology = PrimitiveTopology::TriangleList;
    pipelineDesc.cullMode = CullMode::None;
    pipelineDesc.depthTestEnabled = true;
    pipelineDesc.depthWriteEnabled = false;
    pipelineDesc.blendMode = BlendMode::AlphaBlend;
    pipelineDesc.depthFunc = CompareFunction::Less;
    pipelineDesc.vertexLayout = {
        {"POSITION", Format::R32G32B32_FLOAT, 0, 0},
        {"NORMAL",   Format::R32G32B32_FLOAT, 12, 0},
        {"TEXCOORD", Format::R32G32_FLOAT, 24, 0},
    };

    m_pipeline = renderer->CreatePipelineState(pipelineDesc);
    if (m_pipeline == kInvalidHandle) {
        LOG_ERROR("Failed to create particle pipeline");
        return;
    }

    m_cubeMesh = renderer->CreateMesh(cube);
    m_initialized = true;

    LOG_INFO("ParticleSystem initialized");
}

void ParticleSystem::Shutdown() {
    m_particles.clear();
    m_initialized = false;
}

void ParticleSystem::SpawnBreakParticles(const Vec3& worldPos, const Vec3& blockColor, i32 count) {
    for (i32 i = 0; i < count; ++i) {
        Particle p;
        p.position = worldPos;
        p.velocity = Vec3(
            (f32)(std::rand() % 100 - 50) * 0.1f,
            (f32)(std::rand() % 80) * 0.1f + 0.5f,
            (f32)(std::rand() % 100 - 50) * 0.1f
        );
        p.color = blockColor;
        p.maxLife = 0.5f + (f32)(std::rand() % 50) * 0.01f;
        p.life = p.maxLife;
        p.size = 0.05f + (f32)(std::rand() % 50) * 0.002f;
        m_particles.push_back(p);
    }
}

void ParticleSystem::Update(f32 deltaTime) {
    deltaTime = std::min(deltaTime, 0.05f);

    for (auto it = m_particles.begin(); it != m_particles.end(); ) {
        it->life -= deltaTime;
        if (it->life <= 0.0f) {
            it = m_particles.erase(it);
            continue;
        }

        it->velocity.y += -15.0f * deltaTime; // Gravity
        it->position += it->velocity * deltaTime;
        it->velocity *= 0.95f; // Damping

        ++it;
    }
}

void ParticleSystem::Render(Renderer* renderer) {
    if (!m_initialized || m_particles.empty()) return;

    renderer->BindPipelineState(m_pipeline);

    for (auto& p : m_particles) {
        f32 alpha = p.life / p.maxLife;

        renderer->SetUniformVec3("u_ParticlePos", p.position);
        renderer->SetUniformVec3("u_ParticleColor", p.color);
        renderer->SetUniformFloat("u_ParticleSize", p.size);
        renderer->SetUniformFloat("u_ParticleAlpha", alpha);

        renderer->DrawMesh(m_cubeMesh);
    }
}

} // namespace painkiller
