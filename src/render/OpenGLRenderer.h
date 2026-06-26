#pragma once

#include "Renderer.h"
#include "RenderTypes.h"
#include "Vector.h"
#include "Matrix.h"
#include <GL/gl.h>
#include <GL/glext.h>
#include <string>
#include <unordered_map>
#include <vector>

namespace nebula {

// ?? OpenGL Renderer: implements the Renderer interface ??
class OpenGLRenderer : public Renderer {
public:
    OpenGLRenderer() = default;
    ~OpenGLRenderer() override { Shutdown(); }

    // ?? Lifecycle ??
    bool Initialize(u32 width, u32 height, void* windowHandle) override;
    void Shutdown() override;

    // ?? Frame Control ??
    void BeginFrame() override;
    void EndFrame() override;
    void Present() override;

    // ?? Clearing ??
    void ClearColor(f32 r, f32 g, f32 b, f32 a) override;
    void ClearDepth(f32 value = 1.0f) override;

    // ?? Viewport ??
    void SetViewport(const ViewportDesc& desc) override;

    // ?? Resource Creation ??
    ShaderHandle   CreateShader(const ShaderDesc& desc) override;
    BufferHandle   CreateBuffer(const BufferDesc& desc) override;
    TextureHandle  CreateTexture(const TextureDesc& desc) override;
    PipelineHandle CreatePipelineState(const PipelineStateDesc& desc) override;
    MeshHandle     CreateMesh(const MeshData& data) override;

    // ?? Resource Destruction ??
    void DestroyShader(ShaderHandle handle) override;
    void DestroyBuffer(BufferHandle handle) override;
    void DestroyTexture(TextureHandle handle) override;
    void DestroyPipelineState(PipelineHandle handle) override;
    void DestroyMesh(MeshHandle handle) override;

    // ?? Binding ??
    void BindPipelineState(PipelineHandle handle) override;
    void BindVertexBuffer(BufferHandle handle, u32 slot) override;
    void BindIndexBuffer(BufferHandle handle) override;
    void BindTexture(TextureHandle handle, u32 slot) override;
    void BindMesh(MeshHandle handle) override;

    // ?? Uniforms ??
    void SetUniformBuffer(BufferHandle handle, u32 slot) override;
    void SetUniformMat4(const char* name, const Mat4& matrix) override;
    void SetUniformVec3(const char* name, const Vec3& value) override;
    void SetUniformVec4(const char* name, const Vec4& value) override;
    void SetUniformFloat(const char* name, f32 value) override;
    void SetUniformInt(const char* name, i32 value) override;

    // ?? Draw Calls ??
    void Draw(u32 vertexCount, u32 startVertex = 0) override;
    void DrawIndexed(u32 indexCount, u32 startIndex = 0, i32 baseVertex = 0) override;
    void DrawMesh(MeshHandle mesh) override;

    // ?? Window Resize ??
    void OnResize(u32 width, u32 height) override;

    // ?? Backend Info ??
    RenderBackend GetBackendType() const override { return RenderBackend::OpenGL; }
    const char* GetBackendName() const override { return "OpenGL 3.3 Core"; }

    // ?? Factory helper ??
    static OpenGLRenderer* Create() { return new OpenGLRenderer(); }

private:
    // ?? Internal Structures ??
    struct GLShader {
        ShaderStage stage;
        u32 glHandle = 0;
    };

    struct GLBuffer {
        BufferType type;
        u32 glHandle = 0;
        u32 size = 0;
        u32 stride = 0;
    };

    struct GLTexture {
        u32 glHandle = 0;
        u32 width = 0;
        u32 height = 0;
        Format format;
    };

    struct GLPipeline {
        u32 programHandle = 0;
        u32 vaoHandle = 0;
        PrimitiveTopology topology;
        // Cached uniform locations
        std::unordered_map<std::string, i32> uniformLocations;
    };

    struct GLMesh {
        u32 vaoHandle = 0;
        u32 vertexBuffer = 0;
        u32 indexBuffer = 0;
        u32 vertexCount = 0;
        u32 indexCount = 0;
        u32 vertexStride = 0;
    };

    // ?? Internal Helpers ??
    u32 CompileGLShader(ShaderStage stage, const std::string& source);
    bool LinkProgram(u32 programHandle, u32 vsHandle, u32 fsHandle);
    i32 GetUniformLocation(u32 program, const char* name);
    u32 GetGLTopology(PrimitiveTopology topology) const;
    u32 GetGLBufferType(BufferType type) const;

    // ?? Resource Storage ??
    std::vector<GLShader>   m_shaders;
    std::vector<GLBuffer>   m_buffers;
    std::vector<GLTexture>  m_textures;
    std::vector<GLPipeline> m_pipelines;
    std::vector<GLMesh>     m_meshes;

    // ?? Current State ??
    GLPipeline* m_activePipeline = nullptr;
    u32 m_width = 0;
    u32 m_height = 0;
    bool m_initialized = false;

    // ?? Extension Function Pointers ??
    PFNGLGENBUFFERSPROC glGenBuffers;
    PFNGLBINDBUFFERPROC glBindBuffer;
    PFNGLBUFFERDATAPROC glBufferData;
    PFNGLBUFFERSUBDATAPROC glBufferSubData;
    PFNGLDELETEBUFFERSPROC glDeleteBuffers;
    PFNGLGENVERTEXARRAYSPROC glGenVertexArrays;
    PFNGLBINDVERTEXARRAYPROC glBindVertexArray;
    PFNGLDELETEVERTEXARRAYSPROC glDeleteVertexArrays;
    PFNGLENABLEVERTEXATTRIBARRAYPROC glEnableVertexAttribArray;
    PFNGLVERTEXATTRIBPOINTERPROC glVertexAttribPointer;
    PFNGLVERTEXATTRIBIPOINTERPROC glVertexAttribIPointer;
    PFNGLDISABLEVERTEXATTRIBARRAYPROC glDisableVertexAttribArray;
    PFNGLCREATESHADERPROC glCreateShader;
    PFNGLSHADERSOURCEPROC glShaderSource;
    PFNGLCOMPILESHADERPROC glCompileShader;
    PFNGLGETSHADERIVPROC glGetShaderiv;
    PFNGLGETSHADERINFOLOGPROC glGetShaderInfoLog;
    PFNGLDELETESHADERPROC glDeleteShader;
    PFNGLCREATEPROGRAMPROC glCreateProgram;
    PFNGLATTACHSHADERPROC glAttachShader;
    PFNGLLINKPROGRAMPROC glLinkProgram;
    PFNGLGETPROGRAMIVPROC glGetProgramiv;
    PFNGLGETPROGRAMINFOLOGPROC glGetProgramInfoLog;
    PFNGLDELETEPROGRAMPROC glDeleteProgram;
    PFNGLUSEPROGRAMPROC glUseProgram;
    PFNGLGETUNIFORMLOCATIONPROC glGetUniformLocation;
    PFNGLUNIFORM1IPROC glUniform1i;
    PFNGLUNIFORM1FPROC glUniform1f;
    PFNGLUNIFORM3FVPROC glUniform3fv;
    PFNGLUNIFORM4FVPROC glUniform4fv;
    PFNGLUNIFORMMATRIX4FVPROC glUniformMatrix4fv;
    PFNGLGENFRAMEBUFFERSPROC glGenFramebuffers;
    PFNGLBINDFRAMEBUFFERPROC glBindFramebuffer;
    PFNGLDELETEFRAMEBUFFERSPROC glDeleteFramebuffers;
    PFNGLGENRENDERBUFFERSPROC glGenRenderbuffers;
    PFNGLBINDRENDERBUFFERPROC glBindRenderbuffer;
    PFNGLDELETERENDERBUFFERSPROC glDeleteRenderbuffers;
    PFNGLRENDERBUFFERSTORAGEPROC glRenderbufferStorage;
    PFNGLFRAMEBUFFERRENDERBUFFERPROC glFramebufferRenderbuffer;
    PFNGLGENRENDERBUFFERSPROC glGenRenderbuffersEXT;
    PFNGLBINDRENDERBUFFERPROC glBindRenderbufferEXT;
    PFNGLDELETERENDERBUFFERSPROC glDeleteRenderbuffersEXT;
    PFNGLRENDERBUFFERSTORAGEPROC glRenderbufferStorageEXT;
    PFNGLFRAMEBUFFERRENDERBUFFERPROC glFramebufferRenderbufferEXT;
    PFNGLCHECKFRAMEBUFFERSTATUSPROC glCheckFramebufferStatus;
    PFNGLACTIVETEXTUREPROC glActiveTexture;
    PFNGLGENTEXTURESPROC glGenTextures;
    PFNGLBINDTEXTUREPROC glBindTexture;
    PFNGLDELETETEXTURESPROC glDeleteTextures;
    PFNGLTEXIMAGE2DPROC glTexImage2D;
    PFNGLTEXPARAMETERIPROC glTexParameteri;
    PFNGLTEXPARAMETERFPROC glTexParameterf;
    PFNGLGENERATEMIPMAPPROC glGenerateMipmap;
    PFNGLDRAWELEMENTSPROC glDrawElements;
};

} // namespace nebula
