#pragma once

#include "Renderer.h"
#include "RenderTypes.h"
#include "Vector.h"
#include "Matrix.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

// Include GL types and constants (not prototypes - we load them dynamically)
#include <GL/gl.h>

#include <string>
#include <unordered_map>
#include <vector>

namespace painkiller {

class OpenGLRenderer : public Renderer {
public:
    OpenGLRenderer() = default;
    ~OpenGLRenderer() override { Shutdown(); }

    bool Initialize(u32 width, u32 height, void* windowHandle) override;
    void Shutdown() override;
    void BeginFrame() override;
    void EndFrame() override;
    void Present() override;
    void ClearColor(f32 r, f32 g, f32 b, f32 a) override;
    void ClearDepth(f32 value = 1.0f) override;
    void SetViewport(const ViewportDesc& desc) override;

    ShaderHandle   CreateShader(const ShaderDesc& desc) override;
    BufferHandle   CreateBuffer(const BufferDesc& desc) override;
    TextureHandle  CreateTexture(const TextureDesc& desc) override;
    PipelineHandle CreatePipelineState(const PipelineStateDesc& desc) override;
    MeshHandle     CreateMesh(const MeshData& data) override;

    void DestroyShader(ShaderHandle handle) override;
    void DestroyBuffer(BufferHandle handle) override;
    void DestroyTexture(TextureHandle handle) override;
    void DestroyPipelineState(PipelineHandle handle) override;
    void DestroyMesh(MeshHandle handle) override;

    void BindPipelineState(PipelineHandle handle) override;
    void BindVertexBuffer(BufferHandle handle, u32 slot) override;
    void BindIndexBuffer(BufferHandle handle) override;
    void BindTexture(TextureHandle handle, u32 slot) override;
    void BindMesh(MeshHandle handle) override;

    void SetUniformBuffer(BufferHandle handle, u32 slot) override;
    void SetUniformMat4(const char* name, const Mat4& matrix) override;
    void SetUniformVec3(const char* name, const Vec3& value) override;
    void SetUniformVec4(const char* name, const Vec4& value) override;
    void SetUniformFloat(const char* name, f32 value) override;
    void SetUniformInt(const char* name, i32 value) override;

    void Draw(u32 vertexCount, u32 startVertex = 0) override;
    void DrawIndexed(u32 indexCount, u32 startIndex = 0, i32 baseVertex = 0) override;
    void DrawMesh(MeshHandle mesh) override;
    void OnResize(u32 width, u32 height) override;

    RenderBackend GetBackendType() const override { return RenderBackend::OpenGL; }
    const char* GetBackendName() const override { return "OpenGL 3.3 Core"; }

    static OpenGLRenderer* Create() { return new OpenGLRenderer(); }

private:
    struct GLShader   { ShaderStage stage; u32 glHandle = 0; };
    struct GLBuffer   { BufferType type; u32 glHandle = 0; u32 size = 0; u32 stride = 0; };
    struct GLTexture  { u32 glHandle = 0; u32 width = 0; u32 height = 0; Format format;  };
    struct GLPipeline { u32 programHandle = 0; u32 vaoHandle = 0; PrimitiveTopology topology; };
    struct GLMesh     { u32 vaoHandle = 0; u32 vertexBuffer = 0; u32 indexBuffer = 0; u32 vertexCount = 0; u32 indexCount = 0; u32 vertexStride = 0; };

    u32 CompileGLShader(ShaderStage stage, const std::string& source);
    bool LinkProgram(u32 programHandle, u32 vsHandle, u32 fsHandle);
    u32 GetGLTopology(PrimitiveTopology topology) const;
    u32 GetGLBufferType(BufferType type) const;

    std::vector<GLShader>   m_shaders;
    std::vector<GLBuffer>   m_buffers;
    std::vector<GLTexture>  m_textures;
    std::vector<GLPipeline> m_pipelines;
    std::vector<GLMesh>     m_meshes;

    GLPipeline* m_activePipeline = nullptr;
    u32 m_width = 0;
    u32 m_height = 0;
    bool m_initialized = false;
};

} // namespace painkiller


