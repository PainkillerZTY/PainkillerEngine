#include "OpenGLRenderer.h"
#include "Logger.h"

// ?? Manually define modern GL constants/extensions (MinGW too old) ??
#ifdef GL_GLEXT_PROTOTYPES
#undef GL_GLEXT_PROTOTYPES
#endif
#include <GL/gl.h>

// GL 1.5+ constants
#ifndef GL_ARRAY_BUFFER
#define GL_ARRAY_BUFFER 0x8892
#endif
#ifndef GL_ELEMENT_ARRAY_BUFFER
#define GL_ELEMENT_ARRAY_BUFFER 0x8893
#endif
#ifndef GL_UNIFORM_BUFFER
#define GL_UNIFORM_BUFFER 0x8A11
#endif
#ifndef GL_STATIC_DRAW
#define GL_STATIC_DRAW 0x88E4
#endif
#ifndef GL_DYNAMIC_DRAW
#define GL_DYNAMIC_DRAW 0x88E8
#endif

// GL 2.0 constants
#ifndef GL_VERTEX_SHADER
#define GL_VERTEX_SHADER 0x8B31
#endif
#ifndef GL_FRAGMENT_SHADER
#define GL_FRAGMENT_SHADER 0x8B30
#endif
#ifndef GL_COMPILE_STATUS
#define GL_COMPILE_STATUS 0x8B81
#endif
#ifndef GL_LINK_STATUS
#define GL_LINK_STATUS 0x8B82
#endif
#ifndef GL_INFO_LOG_LENGTH
#define GL_INFO_LOG_LENGTH 0x8B84
#endif

// Missing GL types (MinGW 6.3 GL/gl.h doesn't define these)
#ifndef GLsizeiptr
#define GLsizeiptr ptrdiff_t
#endif
#ifndef GLintptr
#define GLintptr ptrdiff_t
#endif
#ifndef GLchar
#define GLchar char
#endif

// GL 3.0 constants
#ifndef GL_RGBA32F
#define GL_RGBA32F 0x8814
#endif
#ifndef GL_RGB32F
#define GL_RGB32F 0x8815
#endif
#ifndef GL_DEPTH_COMPONENT32F
#define GL_DEPTH_COMPONENT32F 0x8CAC
#endif

// GL constant for texture units
#ifndef GL_TEXTURE0
#define GL_TEXTURE0 0x84C0
#endif

// WGL ARB create context constants
#ifndef WGL_CONTEXT_MAJOR_VERSION_ARB
#define WGL_CONTEXT_MAJOR_VERSION_ARB 0x2091
#define WGL_CONTEXT_MINOR_VERSION_ARB 0x2092
#define WGL_CONTEXT_FLAGS_ARB 0x2094
#define WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB 0x00000002
#define WGL_CONTEXT_PROFILE_MASK_ARB 0x9126
#define WGL_CONTEXT_CORE_PROFILE_BIT_ARB 0x00000001
#endif

namespace nebula {

// ?? Function pointer declarations ??
static void (APIENTRY *pfnDeleteBuffers)(GLsizei, const GLuint*);
static void (APIENTRY *pfnGenBuffers)(GLsizei, GLuint*);
static void (APIENTRY *pfnBindBuffer)(GLenum, GLuint);
static void (APIENTRY *pfnBufferData)(GLenum, GLsizeiptr, const GLvoid*, GLenum);
static void (APIENTRY *pfnBufferSubData)(GLenum, GLintptr, GLsizeiptr, const GLvoid*);
static void (APIENTRY *pfnGenVertexArrays)(GLsizei, GLuint*);
static void (APIENTRY *pfnBindVertexArray)(GLuint);
static void (APIENTRY *pfnDeleteVertexArrays)(GLsizei, const GLuint*);
static void (APIENTRY *pfnEnableVertexAttribArray)(GLuint);
static void (APIENTRY *pfnDisableVertexAttribArray)(GLuint);
static void (APIENTRY *pfnVertexAttribPointer)(GLuint, GLint, GLenum, GLboolean, GLsizei, const GLvoid*);
static GLuint (APIENTRY *pfnCreateShader)(GLenum);
static void (APIENTRY *pfnShaderSource)(GLuint, GLsizei, const GLchar**, const GLint*);
static void (APIENTRY *pfnCompileShader)(GLuint);
static void (APIENTRY *pfnGetShaderiv)(GLuint, GLenum, GLint*);
static void (APIENTRY *pfnGetShaderInfoLog)(GLuint, GLsizei, GLsizei*, GLchar*);
static void (APIENTRY *pfnDeleteShader)(GLuint);
static GLuint (APIENTRY *pfnCreateProgram)(void);
static void (APIENTRY *pfnAttachShader)(GLuint, GLuint);
static void (APIENTRY *pfnLinkProgram)(GLuint);
static void (APIENTRY *pfnGetProgramiv)(GLuint, GLenum, GLint*);
static void (APIENTRY *pfnGetProgramInfoLog)(GLuint, GLsizei, GLsizei*, GLchar*);
static void (APIENTRY *pfnDeleteProgram)(GLuint);
static void (APIENTRY *pfnUseProgram)(GLuint);
static GLint (APIENTRY *pfnGetUniformLocation)(GLuint, const GLchar*);
static void (APIENTRY *pfnUniform1i)(GLint, GLint);
static void (APIENTRY *pfnUniform1f)(GLint, GLfloat);
static void (APIENTRY *pfnUniform3fv)(GLint, GLsizei, const GLfloat*);
static void (APIENTRY *pfnUniform4fv)(GLint, GLsizei, const GLfloat*);
static void (APIENTRY *pfnUniformMatrix4fv)(GLint, GLsizei, GLboolean, const GLfloat*);
static void (APIENTRY *pfnActiveTexture)(GLenum);
static void (APIENTRY *pfnGenTextures)(GLsizei, GLuint*);
static void (APIENTRY *pfnBindTexture)(GLenum, GLuint);
static void (APIENTRY *pfnDeleteTextures)(GLsizei, const GLuint*);
static void (APIENTRY *pfnTexImage2D)(GLenum, GLint, GLint, GLsizei, GLsizei, GLint, GLenum, GLenum, const GLvoid*);
static void (APIENTRY *pfnTexParameteri)(GLenum, GLenum, GLint);
static void (APIENTRY *pfnTexParameterf)(GLenum, GLenum, GLfloat);
static void (APIENTRY *pfnGenerateMipmap)(GLenum);
static void (APIENTRY *pfnDrawElements)(GLenum, GLsizei, GLenum, const GLvoid*);
static void (APIENTRY *pfnBindBufferBase)(GLenum, GLuint, GLuint);
static void (APIENTRY *pfnDrawElementsBaseVertex)(GLenum, GLsizei, GLenum, const GLvoid*, GLint);

#define LOAD_FN(name) do { \
    pfn##name = (decltype(pfn##name))wglGetProcAddress("gl" #name); \
    if(!pfn##name) { \
        static HMODULE hGL = GetModuleHandleA("opengl32.dll"); \
        if(hGL) pfn##name = (decltype(pfn##name))GetProcAddress(hGL, "gl" #name); \
    } \
    if(!pfn##name) LOG_WARN("gl{} not loaded, may crash", #name); \
} while(0)

static bool LoadGLFunctions() {
    LOAD_FN(GenBuffers); LOAD_FN(BindBuffer); LOAD_FN(BufferData); LOAD_FN(BufferSubData);
    LOAD_FN(DeleteBuffers); LOAD_FN(GenVertexArrays); LOAD_FN(BindVertexArray);
    LOAD_FN(DeleteVertexArrays); LOAD_FN(EnableVertexAttribArray); LOAD_FN(DisableVertexAttribArray);
    LOAD_FN(VertexAttribPointer); LOAD_FN(CreateShader); LOAD_FN(ShaderSource);
    LOAD_FN(CompileShader); LOAD_FN(GetShaderiv); LOAD_FN(GetShaderInfoLog); LOAD_FN(DeleteShader);
    LOAD_FN(CreateProgram); LOAD_FN(AttachShader); LOAD_FN(LinkProgram); LOAD_FN(GetProgramiv);
    LOAD_FN(GetProgramInfoLog); LOAD_FN(DeleteProgram); LOAD_FN(UseProgram);
    LOAD_FN(GetUniformLocation); LOAD_FN(Uniform1i); LOAD_FN(Uniform1f); LOAD_FN(Uniform3fv);
    LOAD_FN(Uniform4fv); LOAD_FN(UniformMatrix4fv);
    LOAD_FN(ActiveTexture); LOAD_FN(GenTextures); LOAD_FN(BindTexture); LOAD_FN(DeleteTextures);
    LOAD_FN(TexImage2D); LOAD_FN(TexParameteri); LOAD_FN(TexParameterf); LOAD_FN(GenerateMipmap);
    LOAD_FN(DrawElements); LOAD_FN(BindBufferBase); LOAD_FN(DrawElementsBaseVertex);
    return true;
}

bool OpenGLRenderer::Initialize(u32 width, u32 height, void* windowHandle) {
    LOG_INFO("Initializing OpenGL 3.3 Core...");
    m_width = width; m_height = height;
    HWND hwnd = (HWND)windowHandle;
    HDC hdc = GetDC(hwnd);
    PIXELFORMATDESCRIPTOR pfd = { sizeof(PIXELFORMATDESCRIPTOR), 1, PFD_DRAW_TO_WINDOW|PFD_SUPPORT_OPENGL|PFD_DOUBLEBUFFER, PFD_TYPE_RGBA, 32, 0,0,0,0,0,0,0,0,0,0,0,0,0, 24, 8, 0, PFD_MAIN_PLANE, 0, 0, 0, 0 };
    int pf = ChoosePixelFormat(hdc, &pfd);
    if (!pf || !SetPixelFormat(hdc, pf, &pfd)) { LOG_ERROR("Pixel format failed"); return false; }
    HGLRC temp = wglCreateContext(hdc);
    if (!temp) { LOG_ERROR("wglCreateContext failed"); return false; }
    wglMakeCurrent(hdc, temp);
    LoadGLFunctions();  // Warnings logged inside
    typedef HGLRC(WINAPI *WCARB)(HDC, HGLRC, const int*);
    WCARB wglCtx = (WCARB)wglGetProcAddress("wglCreateContextAttribsARB");
    HGLRC ctx = nullptr;
    if (wglCtx) { int a[] = { WGL_CONTEXT_MAJOR_VERSION_ARB,3, WGL_CONTEXT_MINOR_VERSION_ARB,3, WGL_CONTEXT_FLAGS_ARB,WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB, WGL_CONTEXT_PROFILE_MASK_ARB,WGL_CONTEXT_CORE_PROFILE_BIT_ARB, 0 }; ctx = wglCtx(hdc, 0, a); }
    if (!ctx) { ctx = wglCreateContext(hdc); LOG_WARN("Legacy GL context"); }
    wglMakeCurrent(0,0); wglDeleteContext(temp); wglMakeCurrent(hdc, ctx);
    typedef BOOL(WINAPI *SWAP)(int); SWAP si = (SWAP)wglGetProcAddress("wglSwapIntervalEXT"); if (si) si(1);
    glEnable(GL_DEPTH_TEST); glDepthFunc(GL_LESS); glEnable(GL_CULL_FACE); glCullFace(GL_BACK); glFrontFace(GL_CCW);
    glClearColor(0.1f,0.1f,0.15f,1.0f);
    LOG_INFO("OpenGL: {}", (const char*)glGetString(GL_VERSION));
    m_initialized = true; return true;
}

void OpenGLRenderer::Shutdown() {
    if(!m_initialized) return;
    for(auto& m:m_meshes){if(m.vertexBuffer)pfnDeleteBuffers(1,&m.vertexBuffer);if(m.indexBuffer)pfnDeleteBuffers(1,&m.indexBuffer);if(m.vaoHandle)pfnDeleteVertexArrays(1,&m.vaoHandle);} m_meshes.clear();
    for(auto& p:m_pipelines){if(p.programHandle)pfnDeleteProgram(p.programHandle);if(p.vaoHandle)pfnDeleteVertexArrays(1,&p.vaoHandle);} m_pipelines.clear();
    for(auto& t:m_textures){if(t.glHandle)pfnDeleteTextures(1,&t.glHandle);} m_textures.clear();
    for(auto& b:m_buffers){if(b.glHandle)pfnDeleteBuffers(1,&b.glHandle);} m_buffers.clear();
    for(auto& s:m_shaders){if(s.glHandle)pfnDeleteShader(s.glHandle);} m_shaders.clear();
    m_activePipeline=nullptr; m_initialized=false;
}

void OpenGLRenderer::BeginFrame(){}
void OpenGLRenderer::EndFrame(){glFlush();}
void OpenGLRenderer::Present(){SwapBuffers(wglGetCurrentDC());}
void OpenGLRenderer::ClearColor(f32 r,f32 g,f32 b,f32 a){glClearColor(r,g,b,a);glClear(GL_COLOR_BUFFER_BIT);}
void OpenGLRenderer::ClearDepth(f32 v){glClearDepth(v);glClear(GL_DEPTH_BUFFER_BIT);}
void OpenGLRenderer::SetViewport(const ViewportDesc& d){glViewport((GLint)d.x,(GLint)d.y,(GLsizei)d.width,(GLsizei)d.height);}

u32 OpenGLRenderer::CompileGLShader(ShaderStage s, const std::string& src) {
    u32 h=pfnCreateShader(s==ShaderStage::Vertex?GL_VERTEX_SHADER:GL_FRAGMENT_SHADER);
    const char* c=src.c_str(); pfnShaderSource(h,1,&c,0); pfnCompileShader(h);
    GLint ok=0; pfnGetShaderiv(h,GL_COMPILE_STATUS,&ok);
    if(!ok){char b[512];pfnGetShaderInfoLog(h,sizeof(b),0,b);LOG_ERROR("Shader: {}",b);pfnDeleteShader(h);return 0;}
    return h;
}
bool OpenGLRenderer::LinkProgram(u32 p,u32 v,u32 f){pfnAttachShader(p,v);pfnAttachShader(p,f);pfnLinkProgram(p);
    GLint ok=0;pfnGetProgramiv(p,GL_LINK_STATUS,&ok);
    if(!ok){char b[512];pfnGetProgramInfoLog(p,sizeof(b),0,b);LOG_ERROR("Link: {}",b);return false;}return true;}

ShaderHandle OpenGLRenderer::CreateShader(const ShaderDesc& d){
    GLShader s;s.stage=d.stage;s.glHandle=CompileGLShader(d.stage,d.source);
    if(!s.glHandle)return kInvalidHandle;m_shaders.push_back(s);return (ShaderHandle)(m_shaders.size()-1);}
BufferHandle OpenGLRenderer::CreateBuffer(const BufferDesc& d){
    GLBuffer b;b.type=d.type;b.size=d.size;b.stride=d.stride;
    pfnGenBuffers(1,&b.glHandle);pfnBindBuffer(GL_ARRAY_BUFFER,b.glHandle);
    pfnBufferData(GL_ARRAY_BUFFER,d.size,d.initialData,d.usage==ResourceUsage::Dynamic?GL_DYNAMIC_DRAW:GL_STATIC_DRAW);
    pfnBindBuffer(GL_ARRAY_BUFFER,0);m_buffers.push_back(b);return (BufferHandle)(m_buffers.size()-1);}
TextureHandle OpenGLRenderer::CreateTexture(const TextureDesc& d){
    GLTexture t;t.width=d.width;t.height=d.height;t.format=d.format;
    pfnGenTextures(1,&t.glHandle);pfnBindTexture(GL_TEXTURE_2D,t.glHandle);
    GLenum ifmt=GL_RGBA8,df=GL_RGBA,dt=GL_UNSIGNED_BYTE;
    if(t.format==Format::R32G32B32A32_FLOAT){ifmt=GL_RGBA32F;dt=GL_FLOAT;}
    else if(t.format==Format::R32G32B32_FLOAT){ifmt=GL_RGB32F;dt=GL_FLOAT;}
    pfnTexImage2D(GL_TEXTURE_2D,0,(GLint)ifmt,(GLsizei)d.width,(GLsizei)d.height,0,df,dt,d.initialData);
    pfnTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
    pfnTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    pfnTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_REPEAT);
    pfnTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_REPEAT);
    if(d.mipLevels>1)pfnGenerateMipmap(GL_TEXTURE_2D);
    pfnBindTexture(GL_TEXTURE_2D,0);m_textures.push_back(t);return (TextureHandle)(m_textures.size()-1);}
PipelineHandle OpenGLRenderer::CreatePipelineState(const PipelineStateDesc& d){
    GLPipeline p;p.topology=d.topology;
    u32 vs=d.vertexShader?CompileGLShader(d.vertexShader->stage,d.vertexShader->source):0;
    u32 fs=d.fragmentShader?CompileGLShader(d.fragmentShader->stage,d.fragmentShader->source):0;
    if(!vs||!fs)return kInvalidHandle;
    p.programHandle=pfnCreateProgram();
    if(!LinkProgram(p.programHandle,vs,fs)){pfnDeleteProgram(p.programHandle);return kInvalidHandle;}
    pfnGenVertexArrays(1,&p.vaoHandle);pfnBindVertexArray(p.vaoHandle);
    for(size_t i=0;i<d.vertexLayout.size();++i){
        const auto& e=d.vertexLayout[i];pfnEnableVertexAttribArray((GLuint)i);
        int cc=3;if(e.format==Format::R32G32B32A32_FLOAT)cc=4;else if(e.format==Format::R32G32_FLOAT)cc=2;else if(e.format==Format::R32_FLOAT)cc=1;
        pfnVertexAttribPointer((GLuint)i,cc,GL_FLOAT,GL_FALSE,0,(void*)(size_t)e.offset);}
    pfnBindVertexArray(0);pfnDeleteShader(vs);pfnDeleteShader(fs);
    m_pipelines.push_back(p);return (PipelineHandle)(m_pipelines.size()-1);}
MeshHandle OpenGLRenderer::CreateMesh(const MeshData& d){
    GLMesh m;m.vertexCount=d.vertexCount;m.indexCount=d.indexCount;m.vertexStride=d.vertexStride;
    pfnGenVertexArrays(1,&m.vaoHandle);pfnBindVertexArray(m.vaoHandle);
    pfnGenBuffers(1,&m.vertexBuffer);pfnBindBuffer(GL_ARRAY_BUFFER,m.vertexBuffer);
    pfnBufferData(GL_ARRAY_BUFFER,d.vertices.size()*sizeof(f32),d.vertices.data(),GL_STATIC_DRAW);
    if(!d.indices.empty()){pfnGenBuffers(1,&m.indexBuffer);pfnBindBuffer(GL_ELEMENT_ARRAY_BUFFER,m.indexBuffer);pfnBufferData(GL_ELEMENT_ARRAY_BUFFER,d.indices.size()*sizeof(u32),d.indices.data(),GL_STATIC_DRAW);}
    u32 s=d.vertexStride;pfnEnableVertexAttribArray(0);pfnVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,s,(void*)0);
    if(s>12){pfnEnableVertexAttribArray(1);pfnVertexAttribPointer(1,3,GL_FLOAT,GL_FALSE,s,(void*)12);}
    if(s>24){pfnEnableVertexAttribArray(2);pfnVertexAttribPointer(2,2,GL_FLOAT,GL_FALSE,s,(void*)24);}
    pfnBindVertexArray(0);pfnBindBuffer(GL_ARRAY_BUFFER,0);m_meshes.push_back(m);return (MeshHandle)(m_meshes.size()-1);}
void OpenGLRenderer::DestroyShader(ShaderHandle h){if(h<m_shaders.size()&&m_shaders[h].glHandle){pfnDeleteShader(m_shaders[h].glHandle);m_shaders[h].glHandle=0;}}
void OpenGLRenderer::DestroyBuffer(BufferHandle h){if(h<m_buffers.size()&&m_buffers[h].glHandle){pfnDeleteBuffers(1,&m_buffers[h].glHandle);m_buffers[h].glHandle=0;}}
void OpenGLRenderer::DestroyTexture(TextureHandle h){if(h<m_textures.size()&&m_textures[h].glHandle){pfnDeleteTextures(1,&m_textures[h].glHandle);m_textures[h].glHandle=0;}}
void OpenGLRenderer::DestroyPipelineState(PipelineHandle h){if(h<m_pipelines.size()){if(m_pipelines[h].programHandle)pfnDeleteProgram(m_pipelines[h].programHandle);if(m_pipelines[h].vaoHandle)pfnDeleteVertexArrays(1,&m_pipelines[h].vaoHandle);}}
void OpenGLRenderer::DestroyMesh(MeshHandle h){if(h<m_meshes.size()){auto& me=m_meshes[h];if(me.vertexBuffer)pfnDeleteBuffers(1,&me.vertexBuffer);if(me.indexBuffer)pfnDeleteBuffers(1,&me.indexBuffer);if(me.vaoHandle)pfnDeleteVertexArrays(1,&me.vaoHandle);}}
void OpenGLRenderer::BindPipelineState(PipelineHandle h){if(h>=m_pipelines.size())return;m_activePipeline=&m_pipelines[h];pfnUseProgram(m_activePipeline->programHandle);pfnBindVertexArray(m_activePipeline->vaoHandle);}
void OpenGLRenderer::BindVertexBuffer(BufferHandle h,u32){if(h<m_buffers.size())pfnBindBuffer(GL_ARRAY_BUFFER,m_buffers[h].glHandle);}
void OpenGLRenderer::BindIndexBuffer(BufferHandle h){if(h<m_buffers.size())pfnBindBuffer(GL_ELEMENT_ARRAY_BUFFER,m_buffers[h].glHandle);}
void OpenGLRenderer::BindTexture(TextureHandle h,u32 s){if(h<m_textures.size()){pfnActiveTexture(GL_TEXTURE0+s);pfnBindTexture(GL_TEXTURE_2D,m_textures[h].glHandle);}}
void OpenGLRenderer::BindMesh(MeshHandle h){if(h<m_meshes.size())pfnBindVertexArray(m_meshes[h].vaoHandle);}
void OpenGLRenderer::SetUniformBuffer(BufferHandle h,u32 s){if(h<m_buffers.size())pfnBindBufferBase(GL_UNIFORM_BUFFER,s,m_buffers[h].glHandle);}
void OpenGLRenderer::SetUniformMat4(const char* n,const Mat4& v){if(!m_activePipeline)return;GLint l=pfnGetUniformLocation(m_activePipeline->programHandle,n);if(l!=-1)pfnUniformMatrix4fv(l,1,GL_FALSE,glm::value_ptr(v));}
void OpenGLRenderer::SetUniformVec3(const char* n,const Vec3& v){if(!m_activePipeline)return;GLint l=pfnGetUniformLocation(m_activePipeline->programHandle,n);if(l!=-1)pfnUniform3fv(l,1,glm::value_ptr(v));}
void OpenGLRenderer::SetUniformVec4(const char* n,const Vec4& v){if(!m_activePipeline)return;GLint l=pfnGetUniformLocation(m_activePipeline->programHandle,n);if(l!=-1)pfnUniform4fv(l,1,glm::value_ptr(v));}
void OpenGLRenderer::SetUniformFloat(const char* n,f32 v){if(!m_activePipeline)return;GLint l=pfnGetUniformLocation(m_activePipeline->programHandle,n);if(l!=-1)pfnUniform1f(l,v);}
void OpenGLRenderer::SetUniformInt(const char* n,i32 v){if(!m_activePipeline)return;GLint l=pfnGetUniformLocation(m_activePipeline->programHandle,n);if(l!=-1)pfnUniform1i(l,v);}
void OpenGLRenderer::Draw(u32 vc,u32 sv){glDrawArrays(GL_TRIANGLES,(GLint)sv,(GLsizei)vc);}
void OpenGLRenderer::DrawIndexed(u32 ic,u32 si,i32 bv){pfnDrawElementsBaseVertex(GL_TRIANGLES,(GLsizei)ic,GL_UNSIGNED_INT,(void*)(size_t)(si*sizeof(u32)),bv);}
void OpenGLRenderer::DrawMesh(MeshHandle h){if(h>=m_meshes.size())return;auto& me=m_meshes[h];pfnBindVertexArray(me.vaoHandle);if(me.indexCount>0)pfnDrawElements(GL_TRIANGLES,(GLsizei)me.indexCount,GL_UNSIGNED_INT,0);else glDrawArrays(GL_TRIANGLES,0,(GLsizei)me.vertexCount);}
void OpenGLRenderer::OnResize(u32 w,u32 h){m_width=w;m_height=h;glViewport(0,0,(GLsizei)w,(GLsizei)h);}
u32 OpenGLRenderer::GetGLTopology(PrimitiveTopology t)const{switch(t){case PrimitiveTopology::TriangleList:return GL_TRIANGLES;case PrimitiveTopology::TriangleStrip:return GL_TRIANGLE_STRIP;case PrimitiveTopology::LineList:return GL_LINES;case PrimitiveTopology::LineStrip:return GL_LINE_STRIP;case PrimitiveTopology::PointList:return GL_POINTS;default:return GL_TRIANGLES;}}
u32 OpenGLRenderer::GetGLBufferType(BufferType t)const{switch(t){case BufferType::Vertex:return GL_ARRAY_BUFFER;case BufferType::Index:return GL_ELEMENT_ARRAY_BUFFER;case BufferType::Constant:return GL_UNIFORM_BUFFER;default:return GL_ARRAY_BUFFER;}}

} // namespace nebula



