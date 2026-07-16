// ============================================================
// DirectX11Renderer.h — DirectX 11 backend
//
// Implements Renderer interface via D3D11.
// Uses shader reflection for uniform-by-name binding.
// Supports: vertex/pixel shaders, buffers, textures,
// pipelines (rasterizer/blend/depth), indexed meshes.
// ============================================================
#pragma once

#include "Renderer.h"
#include "RenderTypes.h"
#include "Vector.h"
#include "Matrix.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <d3d11.h>
#include <d3dcompiler.h>
#include <dxgi1_2.h>
#include <wrl/client.h>

#include <string>
#include <unordered_map>
#include <vector>

namespace painkiller {

// ------------------------------------------------------------------
// D3D11 resource wrappers (ComPtr for automatic lifetime)
// ------------------------------------------------------------------
struct D3D11Shader {
    ShaderStage             stage;
    ShaderHandle            handle = kInvalidHandle;
    Microsoft::WRL::ComPtr<ID3D11VertexShader> vs;
    Microsoft::WRL::ComPtr<ID3D11PixelShader>  ps;
    Microsoft::WRL::ComPtr<ID3DBlob>           bytecode; // kept for reflection / input-layout
};

struct D3D11Buffer {
    BufferType                      type;
    u32                             size   = 0;
    u32                             stride = 0;
    Microsoft::WRL::ComPtr<ID3D11Buffer> buffer;
};

struct D3D11Texture {
    u32                                          width  = 0;
    u32                                          height = 0;
    Format                                       format = Format::R8G8B8A8_UNORM;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srv;
    Microsoft::WRL::ComPtr<ID3D11SamplerState>        sampler;
};

struct D3D11UniformSlot {
    std::string name;
    u32         offset = 0; // byte offset inside the cbuffer
    u32         size   = 0; // byte size
};

struct D3D11Pipeline {
    // Shaders
    Microsoft::WRL::ComPtr<ID3D11VertexShader>   vs;
    Microsoft::WRL::ComPtr<ID3D11PixelShader>    ps;
    Microsoft::WRL::ComPtr<ID3D11InputLayout>    inputLayout;

    // Fixed-function state objects
    Microsoft::WRL::ComPtr<ID3D11RasterizerState>     rasterizer;
    Microsoft::WRL::ComPtr<ID3D11DepthStencilState>    depthStencil;
    Microsoft::WRL::ComPtr<ID3D11BlendState>           blendState;

    // Per-pipeline constant buffers
    Microsoft::WRL::ComPtr<ID3D11Buffer> vsCB;
    Microsoft::WRL::ComPtr<ID3D11Buffer> psCB;

    // Reflection data for uniform-by-name
    std::vector<D3D11UniformSlot> vsSlots;
    std::vector<D3D11UniformSlot> psSlots;

    // Shadow copies (cbuffer contents)
    std::vector<u8> vsShadow;
    std::vector<u8> psShadow;
    bool vsDirty = false;
    bool psDirty = false;

    PrimitiveTopology topology = PrimitiveTopology::TriangleList;
};

struct D3D11Mesh {
    Microsoft::WRL::ComPtr<ID3D11Buffer> vertexBuffer;
    Microsoft::WRL::ComPtr<ID3D11Buffer> indexBuffer;
    u32 vertexCount  = 0;
    u32 indexCount   = 0;
    u32 vertexStride = 0;
    DXGI_FORMAT indexFormat = DXGI_FORMAT_R32_UINT;
};

// ------------------------------------------------------------------
// DirectX11Renderer
// ------------------------------------------------------------------
class DirectX11Renderer : public Renderer {
public:
    DirectX11Renderer()  = default;
    ~DirectX11Renderer() override { Shutdown(); }

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

    RenderBackend GetBackendType() const override { return RenderBackend::DirectX11; }
    const char*   GetBackendName() const override { return "DirectX 11"; }

    static DirectX11Renderer* Create() { return new DirectX11Renderer(); }

private:
    void CreateSwapChainResources(HWND hwnd);
    void DestroySwapChainResources();
    void FlushUniforms();

    // Reflection helper — walks a shader blob's cbuffer to fill uniform slot map
    void ReflectShaderConstants(ID3DBlob* blob,
                                std::vector<D3D11UniformSlot>& outSlots,
                                u32& outCBufferSize);

    // Format conversion helpers
    DXGI_FORMAT ToDXGIFormat(Format fmt) const;
    DXGI_FORMAT ToDXGIFormat(Format fmt, bool forSRV) const;

    // D3D11 resources
    Microsoft::WRL::ComPtr<ID3D11Device>            m_device;
    Microsoft::WRL::ComPtr<ID3D11DeviceContext>      m_context;
    Microsoft::WRL::ComPtr<IDXGISwapChain>           m_swapChain;
    Microsoft::WRL::ComPtr<ID3D11RenderTargetView>   m_renderTargetView;
    Microsoft::WRL::ComPtr<ID3D11Texture2D>          m_depthStencilTexture;
    Microsoft::WRL::ComPtr<ID3D11DepthStencilView>   m_depthStencilView;
    Microsoft::WRL::ComPtr<ID3D11RasterizerState>    m_defaultRasterizer;
    Microsoft::WRL::ComPtr<ID3D11DepthStencilState>  m_defaultDepthStencil;
    Microsoft::WRL::ComPtr<ID3D11BlendState>         m_defaultBlend;

    // Resource arrays (indexed by Handle)
    std::vector<D3D11Shader>   m_shaders;
    std::vector<D3D11Buffer>   m_buffers;
    std::vector<D3D11Texture>  m_textures;
    std::vector<D3D11Pipeline> m_pipelines;
    std::vector<D3D11Mesh>     m_meshes;

    // Currently bound pipeline
    D3D11Pipeline* m_activePipeline = nullptr;

    // Internal constant buffer sizes (cached from reflection)
    static constexpr u32 MAX_CB_SIZE = 4096;

    u32  m_width  = 0;
    u32  m_height = 0;
    HWND m_hwnd   = nullptr;
    bool m_initialized = false;
};

} // namespace painkiller
