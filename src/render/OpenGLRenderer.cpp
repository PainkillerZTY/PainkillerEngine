#include "OpenGLRenderer.h"
#include "Logger.h"
#include <cstring>

namespace nebula {

// ?? Forward declarations for WGL extension loading ??
using WGLCreateContextAttribsARBType = HGLRC(WINAPI*)(HDC, HGLRC, const int*);
using WGLGetExtensionsStringARBType = const char*(WINAPI*)(HDC);
using WGLSwapIntervalEXTType = BOOL(WINAPI*)(int);

static WGLCreateContextAttribsARBType wglCreateContextAttribsARB = nullptr;
static WGLGetExtensionsStringARBType wglGetExtensionsStringARB = nullptr;
static WGLSwapIntervalEXTType wglSwapIntervalEXT = nullptr;

// ?? Generic OpenGL function loader ??
static void* LoadGLFunction(const char* name) {
    void* p = (void*)wglGetProcAddress(name);
    if (!p) {
        HMODULE module = LoadLibraryA("opengl32.dll");
        if (module) {
            p = (void*)GetProcAddress(module, name);
        }
    }
    return p;
}

#define LOAD_GL_FUNC(name) \
    name = (PFNGL##name##PROC)LoadGLFunction("gl" #name); \
    if (!name) { LOG_ERROR("Failed to load gl{}", #name); return false; }

// ?? Initialize ??
bool OpenGLRenderer::Initialize(u32 width, u32 height, void* windowHandle) {
    LOG_INFO("Initializing OpenGL 3.3 Core renderer...");
    m_width = width;
    m_height = height;

    HDC hdc = GetDC((HWND)windowHandle);

    // ?? Set up a temporary pixel format ??
    PIXELFORMATDESCRIPTOR pfd = {};
    pfd.nSize = sizeof(PIXELFORMATDESCRIPTOR);
    pfd.nVersion = 1;
    pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.cColorBits = 32;
    pfd.cDepthBits = 24;
    pfd.cStencilBits = 8;
    pfd.iLayerType = PFD_MAIN_PLANE;

    int pixelFormat = ChoosePixelFormat(hdc, &pfd);
    if (!pixelFormat) {
        LOG_ERROR("ChoosePixelFormat failed");
        return false;
    }

    if (!SetPixelFormat(hdc, pixelFormat, &pfd)) {
        LOG_ERROR("SetPixelFormat failed");
        return false;
    }

    // ?? Create temporary context to load extensions ??
    HGLRC tempContext = wglCreateContext(hdc);
    if (!tempContext) {
        LOG_ERROR("wglCreateContext failed");
        return false;
    }

    wglMakeCurrent(hdc, tempContext);

    // ?? Load WGL extensions ??
    wglCreateContextAttribsARB = (WGLCreateContextAttribsARBType)
        LoadGLFunction("wglCreateContextAttribsARB");
    wglSwapIntervalEXT = (WGLSwapIntervalEXTType)
        LoadGLFunction("wglSwapIntervalEXT");

    if (!wglCreateContextAttribsARB) {
        LOG_WARN("wglCreateContextAttribsARB not available, using legacy context");
    }

    // ?? Create OpenGL 3.3 Core context ??
    HGLRC glContext = nullptr;
    if (wglCreateContextAttribsARB) {
        const int attribs[] = {
            WGL_CONTEXT_MAJOR_VERSION_ARB, 3,
            WGL_CONTEXT_MINOR_VERSION_ARB, 3,
            WGL_CONTEXT_FLAGS_ARB, WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB,
            WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
            0
        };
        glContext = wglCreateContextAttribsARB(hdc, 0, attribs);
    }

    if (!glContext) {
        // Fallback
        LOG_WARN("Falling back to legacy OpenGL context");
        glContext = wglCreateContext(hdc);
    }

    wglMakeCurrent(nullptr, nullptr);
    wglDeleteContext(tempContext);
    wglMakeCurrent(hdc, glContext);

    if (wglSwapIntervalEXT) {
        wglSwapIntervalEXT(1); // Enable vsync
    }

    // ?? Load all OpenGL function pointers ??
    LOAD_GL_FUNC(GenBuffers);
    LOAD_GL_FUNC(BindBuffer);
    LOAD_GL_FUNC(BufferData);
    LOAD_GL_FUNC(BufferSubData);
    LOAD_GL_FUNC(DeleteBuffers);
    LOAD_GL_FUNC(GenVertexArrays);
    LOAD_GL_FUNC(BindVertexArray);
    LOAD_GL_FUNC(DeleteVertexArrays);
    LOAD_GL_FUNC(EnableVertexAttribArray);
    LOAD_GL_FUNC(VertexAttribPointer);
    LOAD_GL_FUNC(VertexAttribIPointer);
    LOAD_GL_FUNC(DisableVertexAttribArray);
    LOAD_GL_FUNC(CreateShader);
    LOAD_GL_FUNC(ShaderSource);
    LOAD_GL_FUNC(CompileShader);
    LOAD_GL_FUNC(GetShaderiv);
    LOAD_GL_FUNC(GetShaderInfoLog);
    LOAD_GL_FUNC(DeleteShader);
    LOAD_GL_FUNC(CreateProgram);
    LOAD_GL_FUNC(AttachShader);
    LOAD_GL_FUNC(LinkProgram);
    LOAD_GL_FUNC(GetProgramiv);
    LOAD_GL_FUNC(GetProgramInfoLog);
    LOAD_GL_FUNC(DeleteProgram);
    LOAD_GL_FUNC(UseProgram);
    LOAD_GL_FUNC(GetUniformLocation);
    LOAD_GL_FUNC(Uniform1i);
    LOAD_GL_FUNC(Uniform1f);
    LOAD_GL_FUNC(Uniform3fv);
    LOAD_GL_FUNC(Uniform4fv);
    LOAD_GL_FUNC(UniformMatrix4fv);
    LOAD_GL_FUNC(ActiveTexture);
    LOAD_GL_FUNC(GenTextures);
    LOAD_GL_FUNC(BindTexture);
    LOAD_GL_FUNC(DeleteTextures);
    LOAD_GL_FUNC(TexImage2D);
    LOAD_GL_FUNC(TexParameteri);
    LOAD_GL_FUNC(TexParameterf);
    LOAD_GL_FUNC(GenerateMipmap);
    LOAD_GL_FUNC(DrawElements);

    // Framebuffer (optional for now)
    glGenFramebuffers = (PFNGLGENFRAMEBUFFERSPROC)LoadGLFunction("glGenFramebuffers");
    glBindFramebuffer = (PFNGLBINDFRAMEBUFFERPROC)LoadGLFunction("glBindFramebuffer");
    glDeleteFramebuffers = (PFNGLDELETEFRAMEBUFFERSPROC)LoadGLFunction("glDeleteFramebuffers");
    glCheckFramebufferStatus = (PFNGLCHECKFRAMEBUFFERSTATUSPROC)LoadGLFunction("glCheckFramebufferStatus");

    // ?? Set up default OpenGL state ??
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);
    glClearColor(0.1f, 0.1f, 0.15f, 1.0f);

    // ?? Log device info ??
    const char* renderer = (const char*)glGetString(GL_RENDERER);
    const char* vendor = (const char*)glGetString(GL_VENDOR);
    const char* version = (const char*)glGetString(GL_VERSION);
    LOG_INFO("OpenGL Device: {} - {}", vendor ? vendor : "Unknown", renderer ? renderer : "Unknown");
    LOG_INFO("OpenGL Version: {}", version ? version : "Unknown");

    m_initialized = true;
    LOG_INFO("OpenGL renderer initialized successfully ({}x{})", width, height);
    return true;
}

void OpenGLRenderer::Shutdown() {
    if (!m_initialized) return;

    // Destroy all resources
    for (auto& mesh : m_meshes) {
        if (mesh.vertexBuffer) glDeleteBuffers(1, &mesh.vertexBuffer);
        if (mesh.indexBuffer)  glDeleteBuffers(1, &mesh.indexBuffer);
        if (mesh.vaoHandle)    glDeleteVertexArrays(1, &mesh.vaoHandle);
    }
    m_meshes.clear();

    for (auto& pipe : m_pipelines) {
        if (pipe.programHandle) glDeleteProgram(pipe.programHandle);
        if (pipe.vaoHandle)     glDeleteVertexArrays(1, &pipe.vaoHandle);
    }
    m_pipelines.clear();

    for (auto& tex : m_textures) {
        if (tex.glHandle) glDeleteTextures(1, &tex.glHandle);
    }
    m_textures.clear();

    for (auto& buf : m_buffers) {
        if (buf.glHandle) glDeleteBuffers(1, &buf.glHandle);
    }
    m_buffers.clear();

    for (auto& shader : m_shaders) {
        if (shader.glHandle) glDeleteShader(shader.glHandle);
    }
    m_shaders.clear();

    m_activePipeline = nullptr;
    m_initialized = false;
    LOG_INFO("OpenGL renderer shut down");
}

void OpenGLRenderer::BeginFrame() {
    // Nothing special needed for OpenGL immediate mode frame start
}

void OpenGLRenderer::EndFrame() {
    // Flush
    glFlush();
}

void OpenGLRenderer::Present() {
    SwapBuffers(wglGetCurrentDC());
}

void OpenGLRenderer::ClearColor(f32 r, f32 g, f32 b, f32 a) {
    glClearColor(r, g, b, a);
    glClear(GL_COLOR_BUFFER_BIT);
}

void OpenGLRenderer::ClearDepth(f32 value) {
    glClearDepth(value);
    glClear(GL_DEPTH_BUFFER_BIT);
}

void OpenGLRenderer::SetViewport(const ViewportDesc& desc) {
    glViewport((GLint)desc.x, (GLint)desc.y, (GLsizei)desc.width, (GLsizei)desc.height);
}

// ?? Shader Compilation ??
u32 OpenGLRenderer::CompileGLShader(ShaderStage stage, const std::string& source) {
    GLenum glStage;
    switch (stage) {
        case ShaderStage::Vertex:   glStage = GL_VERTEX_SHADER; break;
        case ShaderStage::Fragment: glStage = GL_FRAGMENT_SHADER; break;
        default: LOG_ERROR("Unsupported shader stage"); return 0;
    }

    u32 handle = glCreateShader(glStage);
    const char* src = source.c_str();
    glShaderSource(handle, 1, &src, nullptr);
    glCompileShader(handle);

    GLint success = 0;
    glGetShaderiv(handle, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(handle, sizeof(infoLog), nullptr, infoLog);
        LOG_ERROR("Shader compilation failed ({}): {}", 
                  stage == ShaderStage::Vertex ? "VS" : "FS", infoLog);
        glDeleteShader(handle);
        return 0;
    }

    return handle;
}

bool OpenGLRenderer::LinkProgram(u32 programHandle, u32 vsHandle, u32 fsHandle) {
    glAttachShader(programHandle, vsHandle);
    glAttachShader(programHandle, fsHandle);
    glLinkProgram(programHandle);

    GLint success = 0;
    glGetProgramiv(programHandle, GL_LINK_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetProgramInfoLog(programHandle, sizeof(infoLog), nullptr, infoLog);
        LOG_ERROR("Program linking failed: {}", infoLog);
        return false;
    }

    return true;
}

i32 OpenGLRenderer::GetUniformLocation(u32 program, const char* name) {
    return glGetUniformLocation((GLuint)program, name);
}

// ?? Resource Creation ??
ShaderHandle OpenGLRenderer::CreateShader(const ShaderDesc& desc) {
    GLShader shader;
    shader.stage = desc.stage;
    shader.glHandle = CompileGLShader(desc.stage, desc.source);
    
    if (!shader.glHandle) {
        LOG_ERROR("Failed to create shader");
        return kInvalidHandle;
    }

    m_shaders.push_back(shader);
    return (ShaderHandle)(m_shaders.size() - 1);
}

BufferHandle OpenGLRenderer::CreateBuffer(const BufferDesc& desc) {
    GLBuffer buffer;
    buffer.type = desc.type;
    buffer.size = desc.size;
    buffer.stride = desc.stride;

    glGenBuffers(1, &buffer.glHandle);
    GLenum target = GetGLBufferType(desc.type);
    glBindBuffer(target, buffer.glHandle);
    glBufferData(target, desc.size, desc.initialData, 
                 desc.usage == ResourceUsage::Dynamic ? GL_DYNAMIC_DRAW : GL_STATIC_DRAW);
    glBindBuffer(target, 0);

    m_buffers.push_back(buffer);
    return (BufferHandle)(m_buffers.size() - 1);
}

TextureHandle OpenGLRenderer::CreateTexture(const TextureDesc& desc) {
    GLTexture texture;
    texture.width = desc.width;
    texture.height = desc.height;
    texture.format = desc.format;

    glGenTextures(1, &texture.glHandle);
    glBindTexture(GL_TEXTURE_2D, texture.glHandle);

    GLenum internalFormat = GL_RGBA8;
    GLenum dataFormat = GL_RGBA;
    GLenum dataType = GL_UNSIGNED_BYTE;

    switch (desc.format) {
        case Format::R8G8B8A8_UNORM:
        case Format::R8G8B8A8_SRGB:
            internalFormat = GL_RGBA8;
            dataFormat = GL_RGBA;
            dataType = GL_UNSIGNED_BYTE;
            break;
        case Format::R32G32B32A32_FLOAT:
            internalFormat = GL_RGBA32F;
            dataFormat = GL_RGBA;
            dataType = GL_FLOAT;
            break;
        case Format::R32G32B32_FLOAT:
            internalFormat = GL_RGB32F;
            dataFormat = GL_RGB;
            dataType = GL_FLOAT;
            break;
        case Format::D32_FLOAT:
            internalFormat = GL_DEPTH_COMPONENT32F;
            dataFormat = GL_DEPTH_COMPONENT;
            dataType = GL_FLOAT;
            break;
        default:
            internalFormat = GL_RGBA8;
            dataFormat = GL_RGBA;
            dataType = GL_UNSIGNED_BYTE;
            break;
    }

    glTexImage2D(GL_TEXTURE_2D, 0, (GLint)internalFormat, 
                 (GLsizei)desc.width, (GLsizei)desc.height, 0,
                 dataFormat, dataType, desc.initialData);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    if (desc.mipLevels > 1) {
        glGenerateMipmap(GL_TEXTURE_2D);
    }

    glBindTexture(GL_TEXTURE_2D, 0);

    m_textures.push_back(texture);
    return (TextureHandle)(m_textures.size() - 1);
}

PipelineHandle OpenGLRenderer::CreatePipelineState(const PipelineStateDesc& desc) {
    GLPipeline pipeline;
    pipeline.topology = desc.topology;

    // Create shader program
    u32 vsHandle = 0, fsHandle = 0;
    if (desc.vertexShader) {
        vsHandle = CompileGLShader(desc.vertexShader->stage, desc.vertexShader->source);
    }
    if (desc.fragmentShader) {
        fsHandle = CompileGLShader(desc.fragmentShader->stage, desc.fragmentShader->source);
    }

    if (!vsHandle || !fsHandle) {
        LOG_ERROR("Failed to compile shaders for pipeline");
        return kInvalidHandle;
    }

    pipeline.programHandle = glCreateProgram();
    if (!LinkProgram(pipeline.programHandle, vsHandle, fsHandle)) {
        glDeleteProgram(pipeline.programHandle);
        return kInvalidHandle;
    }

    // Create VAO from vertex layout
    glGenVertexArrays(1, &pipeline.vaoHandle);
    glBindVertexArray(pipeline.vaoHandle);

    u32 location = 0;
    for (const auto& element : desc.vertexLayout) {
        glEnableVertexAttribArray(location);

        GLenum glFormat;
        i32 compCount;
        bool normalized = false;
        switch (element.format) {
            case Format::R32G32B32_FLOAT:
                glFormat = GL_FLOAT; compCount = 3; break;
            case Format::R32G32B32A32_FLOAT:
                glFormat = GL_FLOAT; compCount = 4; break;
            case Format::R32G32_FLOAT:
                glFormat = GL_FLOAT; compCount = 2; break;
            case Format::R32_FLOAT:
                glFormat = GL_FLOAT; compCount = 1; break;
            default:
                glFormat = GL_FLOAT; compCount = 3; break;
        }

        glVertexAttribPointer(location, compCount, glFormat, 
                              normalized ? GL_TRUE : GL_FALSE,
                              desc.vertexLayout[0].offset ? 0 : 0, // stride set on bind
                              (void*)(size_t)element.offset);
        
        // Cache uniform locations
        pipeline.uniformLocations["u_Model"] = -1;
        pipeline.uniformLocations["u_View"] = -1;
        pipeline.uniformLocations["u_Projection"] = -1;
        pipeline.uniformLocations["u_Color"] = -1;
        pipeline.uniformLocations["u_LightDir"] = -1;
        pipeline.uniformLocations["u_LightColor"] = -1;
        pipeline.uniformLocations["u_CameraPos"] = -1;
        pipeline.uniformLocations["u_Time"] = -1;

        location++;
    }

    glBindVertexArray(0);

    // Cleanup compiled shaders (they're linked into the program now)
    glDeleteShader(vsHandle);
    glDeleteShader(fsHandle);

    m_pipelines.push_back(pipeline);
    return (PipelineHandle)(m_pipelines.size() - 1);
}

MeshHandle OpenGLRenderer::CreateMesh(const MeshData& data) {
    GLMesh mesh;
    mesh.vertexCount = data.vertexCount;
    mesh.indexCount = data.indexCount;
    mesh.vertexStride = data.vertexStride;

    glGenVertexArrays(1, &mesh.vaoHandle);
    glBindVertexArray(mesh.vaoHandle);

    // Vertex buffer
    glGenBuffers(1, &mesh.vertexBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, mesh.vertexBuffer);
    glBufferData(GL_ARRAY_BUFFER, data.vertices.size() * sizeof(f32), 
                 data.vertices.data(), GL_STATIC_DRAW);

    // Index buffer
    if (!data.indices.empty()) {
        glGenBuffers(1, &mesh.indexBuffer);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.indexBuffer);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, data.indices.size() * sizeof(u32),
                     data.indices.data(), GL_STATIC_DRAW);
    }

    // Set up vertex attributes (position, normal, texcoord)
    u32 stride = data.vertexStride;
    u32 offset = 0;

    // Position (location 0) - always vec3
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
    offset += 3 * sizeof(f32);

    // Normal (location 1) - vec3 if present
    if (stride > offset) {
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void*)(size_t)offset);
        offset += 3 * sizeof(f32);
    }

    // TexCoord (location 2) - vec2 if present
    if (stride > offset) {
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, (void*)(size_t)offset);
        offset += 2 * sizeof(f32);
    }

    // Tangent (location 3) - vec3 if present
    if (stride > offset + 3 * sizeof(f32)) {
        glEnableVertexAttribArray(3);
        glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, stride, (void*)(size_t)offset);
    }

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    m_meshes.push_back(mesh);
    return (MeshHandle)(m_meshes.size() - 1);
}

// ?? Resource Destruction ??
void OpenGLRenderer::DestroyShader(ShaderHandle handle) {
    if (handle < m_shaders.size() && m_shaders[handle].glHandle) {
        glDeleteShader(m_shaders[handle].glHandle);
        m_shaders[handle].glHandle = 0;
    }
}

void OpenGLRenderer::DestroyBuffer(BufferHandle handle) {
    if (handle < m_buffers.size() && m_buffers[handle].glHandle) {
        glDeleteBuffers(1, &m_buffers[handle].glHandle);
        m_buffers[handle].glHandle = 0;
    }
}

void OpenGLRenderer::DestroyTexture(TextureHandle handle) {
    if (handle < m_textures.size() && m_textures[handle].glHandle) {
        glDeleteTextures(1, &m_textures[handle].glHandle);
        m_textures[handle].glHandle = 0;
    }
}

void OpenGLRenderer::DestroyPipelineState(PipelineHandle handle) {
    if (handle < m_pipelines.size() && m_pipelines[handle].programHandle) {
        glDeleteProgram(m_pipelines[handle].programHandle);
        m_pipelines[handle].programHandle = 0;
        if (m_pipelines[handle].vaoHandle) {
            glDeleteVertexArrays(1, &m_pipelines[handle].vaoHandle);
        }
    }
}

void OpenGLRenderer::DestroyMesh(MeshHandle handle) {
    if (handle < m_meshes.size()) {
        auto& mesh = m_meshes[handle];
        if (mesh.vertexBuffer) glDeleteBuffers(1, &mesh.vertexBuffer);
        if (mesh.indexBuffer)  glDeleteBuffers(1, &mesh.indexBuffer);
        if (mesh.vaoHandle)    glDeleteVertexArrays(1, &mesh.vaoHandle);
        mesh.vertexBuffer = mesh.indexBuffer = mesh.vaoHandle = 0;
    }
}

// ?? Binding ??
void OpenGLRenderer::BindPipelineState(PipelineHandle handle) {
    if (handle >= m_pipelines.size()) return;
    m_activePipeline = &m_pipelines[handle];
    glUseProgram(m_activePipeline->programHandle);
    glBindVertexArray(m_activePipeline->vaoHandle);
}

void OpenGLRenderer::BindVertexBuffer(BufferHandle handle, u32 slot) {
    if (handle >= m_buffers.size()) return;
    glBindBuffer(GL_ARRAY_BUFFER, m_buffers[handle].glHandle);
}

void OpenGLRenderer::BindIndexBuffer(BufferHandle handle) {
    if (handle >= m_buffers.size()) return;
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_buffers[handle].glHandle);
}

void OpenGLRenderer::BindTexture(TextureHandle handle, u32 slot) {
    if (handle >= m_textures.size()) return;
    glActiveTexture(GL_TEXTURE0 + slot);
    glBindTexture(GL_TEXTURE_2D, m_textures[handle].glHandle);
}

void OpenGLRenderer::BindMesh(MeshHandle handle) {
    if (handle >= m_meshes.size()) return;
    auto& mesh = m_meshes[handle];
    glBindVertexArray(mesh.vaoHandle);
}

// ?? Uniforms ??
void OpenGLRenderer::SetUniformBuffer(BufferHandle handle, u32 slot) {
    if (handle >= m_buffers.size()) return;
    GLuint bindingPoint = slot;
    glBindBufferBase(GL_UNIFORM_BUFFER, bindingPoint, m_buffers[handle].glHandle);
}

void OpenGLRenderer::SetUniformMat4(const char* name, const Mat4& matrix) {
    if (!m_activePipeline) return;
    GLint loc = glGetUniformLocation(m_activePipeline->programHandle, name);
    if (loc != -1) {
        glUniformMatrix4fv(loc, 1, GL_FALSE, glm::value_ptr(matrix));
    }
}

void OpenGLRenderer::SetUniformVec3(const char* name, const Vec3& value) {
    if (!m_activePipeline) return;
    GLint loc = glGetUniformLocation(m_activePipeline->programHandle, name);
    if (loc != -1) {
        glUniform3fv(loc, 1, glm::value_ptr(value));
    }
}

void OpenGLRenderer::SetUniformVec4(const char* name, const Vec4& value) {
    if (!m_activePipeline) return;
    GLint loc = glGetUniformLocation(m_activePipeline->programHandle, name);
    if (loc != -1) {
        glUniform4fv(loc, 1, glm::value_ptr(value));
    }
}

void OpenGLRenderer::SetUniformFloat(const char* name, f32 value) {
    if (!m_activePipeline) return;
    GLint loc = glGetUniformLocation(m_activePipeline->programHandle, name);
    if (loc != -1) {
        glUniform1f(loc, value);
    }
}

void OpenGLRenderer::SetUniformInt(const char* name, i32 value) {
    if (!m_activePipeline) return;
    GLint loc = glGetUniformLocation(m_activePipeline->programHandle, name);
    if (loc != -1) {
        glUniform1i(loc, value);
    }
}

// ?? Draw Calls ??
void OpenGLRenderer::Draw(u32 vertexCount, u32 startVertex) {
    glDrawArrays(GL_TRIANGLES, (GLint)startVertex, (GLsizei)vertexCount);
}

void OpenGLRenderer::DrawIndexed(u32 indexCount, u32 startIndex, i32 baseVertex) {
    glDrawElementsBaseVertex(GL_TRIANGLES, (GLsizei)indexCount, GL_UNSIGNED_INT,
                             (void*)(size_t)(startIndex * sizeof(u32)), baseVertex);
}

void OpenGLRenderer::DrawMesh(MeshHandle handle) {
    if (handle >= m_meshes.size()) return;
    auto& mesh = m_meshes[handle];
    glBindVertexArray(mesh.vaoHandle);
    if (mesh.indexCount > 0) {
        glDrawElements(GL_TRIANGLES, (GLsizei)mesh.indexCount, GL_UNSIGNED_INT, nullptr);
    } else {
        glDrawArrays(GL_TRIANGLES, 0, (GLsizei)mesh.vertexCount);
    }
}

void OpenGLRenderer::OnResize(u32 width, u32 height) {
    m_width = width;
    m_height = height;
    glViewport(0, 0, (GLsizei)width, (GLsizei)height);
}

// ?? Private Helpers ??
u32 OpenGLRenderer::GetGLTopology(PrimitiveTopology topology) const {
    switch (topology) {
        case PrimitiveTopology::TriangleList: return GL_TRIANGLES;
        case PrimitiveTopology::TriangleStrip: return GL_TRIANGLE_STRIP;
        case PrimitiveTopology::LineList: return GL_LINES;
        case PrimitiveTopology::LineStrip: return GL_LINE_STRIP;
        case PrimitiveTopology::PointList: return GL_POINTS;
        default: return GL_TRIANGLES;
    }
}

u32 OpenGLRenderer::GetGLBufferType(BufferType type) const {
    switch (type) {
        case BufferType::Vertex:   return GL_ARRAY_BUFFER;
        case BufferType::Index:    return GL_ELEMENT_ARRAY_BUFFER;
        case BufferType::Constant: return GL_UNIFORM_BUFFER;
        default:                   return GL_ARRAY_BUFFER;
    }
}

} // namespace nebula
