#pragma once

#include "Types.h"
#include "RenderTypes.h"
#include "Vector.h"
#include "Matrix.h"
#include <memory>

namespace nebula {

using ShaderHandle = Handle;
using BufferHandle = Handle;
using TextureHandle = Handle;
using PipelineHandle = Handle;
using MeshHandle = Handle;

class Renderer {
public:
    virtual ~Renderer() = default;

    virtual bool Initialize(u32 width, u32 height, void* windowHandle) = 0;
    virtual void Shutdown() = 0;
    virtual void BeginFrame() = 0;
    virtual void EndFrame() = 0;
    virtual void Present() = 0;
    virtual void ClearColor(f32 r, f32 g, f32 b, f32 a) = 0;
    virtual void ClearDepth(f32 value = 1.0f) = 0;
    virtual void SetViewport(const ViewportDesc& desc) = 0;

    virtual ShaderHandle   CreateShader(const ShaderDesc& desc) = 0;
    virtual BufferHandle   CreateBuffer(const BufferDesc& desc) = 0;
    virtual TextureHandle  CreateTexture(const TextureDesc& desc) = 0;
    virtual PipelineHandle CreatePipelineState(const PipelineStateDesc& desc) = 0;
    virtual MeshHandle     CreateMesh(const MeshData& data) = 0;

    virtual void DestroyShader(ShaderHandle handle) = 0;
    virtual void DestroyBuffer(BufferHandle handle) = 0;
    virtual void DestroyTexture(TextureHandle handle) = 0;
    virtual void DestroyPipelineState(PipelineHandle handle) = 0;
    virtual void DestroyMesh(MeshHandle handle) = 0;

    virtual void BindPipelineState(PipelineHandle handle) = 0;
    virtual void BindVertexBuffer(BufferHandle handle, u32 slot) = 0;
    virtual void BindIndexBuffer(BufferHandle handle) = 0;
    virtual void BindTexture(TextureHandle handle, u32 slot) = 0;
    virtual void BindMesh(MeshHandle handle) = 0;

    virtual void SetUniformBuffer(BufferHandle handle, u32 slot) = 0;
    virtual void SetUniformMat4(const char* name, const Mat4& matrix) = 0;
    virtual void SetUniformVec3(const char* name, const Vec3& value) = 0;
    virtual void SetUniformVec4(const char* name, const Vec4& value) = 0;
    virtual void SetUniformFloat(const char* name, f32 value) = 0;
    virtual void SetUniformInt(const char* name, i32 value) = 0;

    virtual void Draw(u32 vertexCount, u32 startVertex = 0) = 0;
    virtual void DrawIndexed(u32 indexCount, u32 startIndex = 0, i32 baseVertex = 0) = 0;
    virtual void DrawMesh(MeshHandle mesh) = 0;
    virtual void OnResize(u32 width, u32 height) = 0;

    virtual RenderBackend GetBackendType() const = 0;
    virtual const char* GetBackendName() const = 0;

    static Renderer* Create(RenderBackend backend = RenderBackend::OpenGL);
};

} // namespace nebula
