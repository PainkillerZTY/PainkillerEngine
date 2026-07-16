# INBOX: Implement DirectX 11 and Vulkan Renderer Backends

## Priority: HIGH
## Estimated Time: 8-12 hours per backend
## Dependencies: Renderer interface (src/render/Renderer.h)

## Task
Implement two new renderer backends:
1. `src/render/DirectX11Renderer.h` + `.cpp`
2. `src/render/VulkanRenderer.h` + `.cpp`

## Interface to Implement
Both must inherit from `Renderer` class:
```
virtual bool Initialize(u32, u32, void*) = 0;
virtual void Shutdown() = 0;
virtual void BeginFrame() = 0;
virtual void EndFrame() = 0;
virtual void Present() = 0;
virtual void ClearColor(f32, f32, f32, f32) = 0;
virtual void ClearDepth(f32) = 0;
virtual ShaderHandle CreateShader(const ShaderDesc&) = 0;
virtual PipelineHandle CreatePipelineState(const PipelineStateDesc&) = 0;
virtual MeshHandle CreateMesh(const MeshData&) = 0;
virtual TextureHandle CreateTexture(const TextureDesc&) = 0;
virtual void DrawMesh(MeshHandle) = 0;
virtual void BindPipelineState(PipelineHandle) = 0;
virtual void SetUniformMat4(const char*, const Mat4&) = 0;
... (see Renderer.h for full interface)
```

## Reference
- Current OpenGL implementation: `src/render/OpenGLRenderer.cpp` (~400 lines)
- OpenGL's VAO/VBO ? DirectX: ID3D11InputLayout + ID3D11Buffer
- OpenGL's Shader ? DirectX: ID3D11VertexShader + ID3D11PixelShader
- OpenGL's Texture ? DirectX: ID3D11ShaderResourceView

## Files to Create
- `src/render/DirectX11Renderer.h` - DirectX 11 header
- `src/render/DirectX11Renderer.cpp` - DirectX 11 implementation
- `src/render/VulkanRenderer.h` - Vulkan header
- `src/render/VulkanRenderer.cpp` - Vulkan implementation

## Verification
- [ ] Both backends compile without errors
- [ ] Basic triangle renders with DirectX 11
- [ ] Basic triangle renders with Vulkan
- [ ] No memory leaks on shutdown

## Notes
- Use modern C++ (C++17)
- No external dependencies beyond DirectX SDK / Vulkan SDK
- Vulkan: Use volk.h for function loading if needed
