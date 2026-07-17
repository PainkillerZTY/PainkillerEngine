// ============================================================
// DirectX11Renderer.cpp — DirectX 11 backend implementation
//
// Full D3D11 realisation of the Renderer interface.
// ============================================================
#include "DirectX11Renderer.h"
#include "Logger.h"

#include <d3d11shader.h>   // D3D11_SHADER_DESC, ID3D11ShaderReflection
#include <array>
#include <cstring>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")

namespace painkiller {

// ==================================================================
// Internal helpers
// ==================================================================

static D3D_PRIMITIVE_TOPOLOGY ToD3DTopology(PrimitiveTopology t) {
    switch (t) {
        case PrimitiveTopology::TriangleList:  return D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
        case PrimitiveTopology::TriangleStrip: return D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;
        case PrimitiveTopology::LineList:      return D3D_PRIMITIVE_TOPOLOGY_LINELIST;
        case PrimitiveTopology::LineStrip:     return D3D_PRIMITIVE_TOPOLOGY_LINESTRIP;
        case PrimitiveTopology::PointList:     return D3D_PRIMITIVE_TOPOLOGY_POINTLIST;
        default: return D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
    }
}

static D3D11_CULL_MODE ToD3DCullMode(CullMode m) {
    switch (m) {
        case CullMode::None:  return D3D11_CULL_NONE;
        case CullMode::Front: return D3D11_CULL_FRONT;
        case CullMode::Back:  return D3D11_CULL_BACK;
        default: return D3D11_CULL_BACK;
    }
}

static D3D11_FILL_MODE ToD3DFillMode(FillMode m) {
    return m == FillMode::Wireframe ? D3D11_FILL_WIREFRAME : D3D11_FILL_SOLID;
}

static D3D11_COMPARISON_FUNC ToD3DCompare(CompareFunction f) {
    switch (f) {
        case CompareFunction::Never:         return D3D11_COMPARISON_NEVER;
        case CompareFunction::Less:          return D3D11_COMPARISON_LESS;
        case CompareFunction::Equal:         return D3D11_COMPARISON_EQUAL;
        case CompareFunction::LessEqual:     return D3D11_COMPARISON_LESS_EQUAL;
        case CompareFunction::Greater:       return D3D11_COMPARISON_GREATER;
        case CompareFunction::NotEqual:      return D3D11_COMPARISON_NOT_EQUAL;
        case CompareFunction::GreaterEqual:  return D3D11_COMPARISON_GREATER_EQUAL;
        case CompareFunction::Always:        return D3D11_COMPARISON_ALWAYS;
        default: return D3D11_COMPARISON_LESS;
    }
}

static D3D11_BLEND ToD3DBlend(BlendMode mode) {
    // We only pass src/dest blend factors through this one helper;
    // full blend-op config is done inside CreatePipelineState.
    switch (mode) {
        case BlendMode::Opaque:      return D3D11_BLEND_ONE;
        case BlendMode::AlphaBlend:  return D3D11_BLEND_SRC_ALPHA;
        case BlendMode::Additive:    return D3D11_BLEND_ONE;
        case BlendMode::Multiply:    return D3D11_BLEND_ZERO;
        default: return D3D11_BLEND_ONE;
    }
}

DXGI_FORMAT DirectX11Renderer::ToDXGIFormat(Format fmt) const {
    return ToDXGIFormat(fmt, false);
}

DXGI_FORMAT DirectX11Renderer::ToDXGIFormat(Format fmt, bool forSRV) const {
    switch (fmt) {
        case Format::R8G8B8A8_UNORM:         return DXGI_FORMAT_R8G8B8A8_UNORM;
        case Format::R8G8B8A8_SRGB:          return DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
        case Format::R32G32B32A32_FLOAT:     return DXGI_FORMAT_R32G32B32A32_FLOAT;
        case Format::R32G32B32_FLOAT:        return DXGI_FORMAT_R32G32B32_FLOAT;
        case Format::R32G32_FLOAT:           return DXGI_FORMAT_R32G32_FLOAT;
        case Format::R32_FLOAT:              return DXGI_FORMAT_R32_FLOAT;
        case Format::D32_FLOAT:              return DXGI_FORMAT_D32_FLOAT;
        case Format::D24_UNORM_S8_UINT:      return DXGI_FORMAT_D24_UNORM_S8_UINT;
        default: return DXGI_FORMAT_UNKNOWN;
    }
}

// ==================================================================
// Shader reflection
// ==================================================================
void DirectX11Renderer::ReflectShaderConstants(ID3DBlob* blob,
                                               std::vector<D3D11UniformSlot>& outSlots,
                                               u32& outCBufferSize)
{
    outSlots.clear();
    outCBufferSize = 0;

    Microsoft::WRL::ComPtr<ID3D11ShaderReflection> refl;
    HRESULT hr = D3DReflect(blob->GetBufferPointer(), blob->GetBufferSize(),
                            IID_PPV_ARGS(&refl));
    if (FAILED(hr)) return;

    D3D11_SHADER_DESC shaderDesc;
    refl->GetDesc(&shaderDesc);

    // Walk constant buffers
    for (UINT cb = 0; cb < shaderDesc.ConstantBuffers; ++cb) {
        ID3D11ShaderReflectionConstantBuffer* cbRefl = refl->GetConstantBufferByIndex(cb);
        D3D11_SHADER_BUFFER_DESC bufDesc;
        cbRefl->GetDesc(&bufDesc);
        outCBufferSize = bufDesc.Size;

        D3D11_SHADER_INPUT_BIND_DESC bindDesc;
        refl->GetResourceBindingDescByName(bufDesc.Name, &bindDesc);

        // Walk variables
        for (UINT v = 0; v < bufDesc.Variables; ++v) {
            ID3D11ShaderReflectionVariable* var = cbRefl->GetVariableByIndex(v);
            D3D11_SHADER_VARIABLE_DESC varDesc;
            var->GetDesc(&varDesc);
            outSlots.push_back({ varDesc.Name, varDesc.StartOffset, varDesc.Size });
        }
    }
}

// ==================================================================
// Initialize
// ==================================================================
bool DirectX11Renderer::Initialize(u32 width, u32 height, void* windowHandle) {
    LOG_INFO("Initializing DirectX 11...");
    m_width  = width;
    m_height = height;
    m_hwnd   = (HWND)windowHandle;

    UINT flags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
#ifdef _DEBUG
    flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    D3D_FEATURE_LEVEL featureLevels[] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_1 };

    // Create device and context
    Microsoft::WRL::ComPtr<ID3D11Device> baseDevice;
    Microsoft::WRL::ComPtr<ID3D11DeviceContext> baseContext;
    D3D_FEATURE_LEVEL selectedLevel;
    HRESULT hr = D3D11CreateDevice(
        nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, flags,
        featureLevels, ARRAYSIZE(featureLevels),
        D3D11_SDK_VERSION, &baseDevice, &selectedLevel, &baseContext);
    if (FAILED(hr)) {
        LOG_ERROR("D3D11CreateDevice failed: {:08x}", hr);
        return false;
    }
    LOG_INFO("D3D11 feature level: {}", int(selectedLevel));

    m_device  = baseDevice;
    m_context = baseContext;

    // Check for debug layer
#ifdef _DEBUG
    Microsoft::WRL::ComPtr<ID3D11Debug> d3dDebug;
    if (SUCCEEDED(m_device.As(&d3dDebug))) {
        Microsoft::WRL::ComPtr<ID3D11InfoQueue> infoQueue;
        if (SUCCEEDED(d3dDebug.As(&infoQueue))) {
            infoQueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_CORRUPTION, TRUE);
            infoQueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_ERROR, TRUE);
        }
    }
#endif

    // Create swap chain
    Microsoft::WRL::ComPtr<IDXGIDevice1> dxgiDevice;
    Microsoft::WRL::ComPtr<IDXGIAdapter> dxgiAdapter;
    Microsoft::WRL::ComPtr<IDXGIFactory2> dxgiFactory;

    hr = m_device.As(&dxgiDevice);
    if (FAILED(hr)) { LOG_ERROR("Failed to get IDXGIDevice"); return false; }
    hr = dxgiDevice->GetAdapter(&dxgiAdapter);
    if (FAILED(hr)) { LOG_ERROR("Failed to get IDXGIAdapter"); return false; }
    hr = dxgiAdapter->GetParent(IID_PPV_ARGS(&dxgiFactory));
    if (FAILED(hr)) { LOG_ERROR("Failed to get IDXGIFactory"); return false; }

    DXGI_SWAP_CHAIN_DESC1 scDesc = {};
    scDesc.Width       = width;
    scDesc.Height      = height;
    scDesc.Format      = DXGI_FORMAT_R8G8B8A8_UNORM;
    scDesc.SampleDesc.Count   = 1;
    scDesc.SampleDesc.Quality = 0;
    scDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    scDesc.BufferCount = 2; // double-buffered
    scDesc.SwapEffect  = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    scDesc.Flags       = 0;

    hr = dxgiFactory->CreateSwapChainForHwnd(m_device.Get(), m_hwnd, &scDesc,
                                              nullptr, nullptr, &m_swapChain);
    if (FAILED(hr)) {
        LOG_ERROR("CreateSwapChainForHwnd failed: {:08x}", hr);
        return false;
    }

    // Prevent DXGI from intercepting Alt+Enter
    dxgiFactory->MakeWindowAssociation(m_hwnd, DXGI_MWA_NO_ALT_ENTER);

    // Create render target + depth from the swap chain
    CreateSwapChainResources(m_hwnd);

    // Default state objects (used as fallbacks)
    {
        D3D11_RASTERIZER_DESC rd = {};
        rd.FillMode              = D3D11_FILL_SOLID;
        rd.CullMode              = D3D11_CULL_BACK;
        rd.FrontCounterClockwise = FALSE;
        rd.DepthClipEnable       = TRUE;
        m_device->CreateRasterizerState(&rd, &m_defaultRasterizer);
    }
    {
        D3D11_DEPTH_STENCIL_DESC dd = {};
        dd.DepthEnable      = TRUE;
        dd.DepthWriteMask   = D3D11_DEPTH_WRITE_MASK_ALL;
        dd.DepthFunc        = D3D11_COMPARISON_LESS;
        dd.StencilEnable    = FALSE;
        m_device->CreateDepthStencilState(&dd, &m_defaultDepthStencil);
    }
    {
        D3D11_BLEND_DESC bd = {};
        bd.AlphaToCoverageEnable  = FALSE;
        bd.IndependentBlendEnable = FALSE;
        D3D11_RENDER_TARGET_BLEND_DESC rt = {};
        rt.BlendEnable           = FALSE;
        rt.SrcBlend              = D3D11_BLEND_ONE;
        rt.DestBlend             = D3D11_BLEND_ZERO;
        rt.BlendOp               = D3D11_BLEND_OP_ADD;
        rt.SrcBlendAlpha         = D3D11_BLEND_ONE;
        rt.DestBlendAlpha        = D3D11_BLEND_ZERO;
        rt.BlendOpAlpha          = D3D11_BLEND_OP_ADD;
        rt.RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
        bd.RenderTarget[0] = rt;
        m_device->CreateBlendState(&bd, &m_defaultBlend);
    }

    m_context->OMSetRenderTargets(1, m_renderTargetView.GetAddressOf(), m_depthStencilView.Get());
    m_context->RSSetState(m_defaultRasterizer.Get());
    m_context->OMSetDepthStencilState(m_defaultDepthStencil.Get(), 0);
    m_context->OMSetBlendState(m_defaultBlend.Get(), nullptr, 0xFFFFFFFF);

    m_activePipeline = nullptr;
    m_initialized = true;
    LOG_INFO("DirectX 11 initialized successfully");
    return true;
}

// ==================================================================
// Swap chain resources (RT + depth)
// ==================================================================
void DirectX11Renderer::CreateSwapChainResources(HWND /*hwnd*/) {
    // Back-buffer → RTV
    Microsoft::WRL::ComPtr<ID3D11Texture2D> backBuffer;
    m_swapChain->GetBuffer(0, IID_PPV_ARGS(&backBuffer));
    m_device->CreateRenderTargetView(backBuffer.Get(), nullptr, &m_renderTargetView);

    // Depth-stencil
    D3D11_TEXTURE2D_DESC depthDesc = {};
    depthDesc.Width          = m_width;
    depthDesc.Height         = m_height;
    depthDesc.MipLevels      = 1;
    depthDesc.ArraySize      = 1;
    depthDesc.Format         = DXGI_FORMAT_D32_FLOAT;
    depthDesc.SampleDesc.Count   = 1;
    depthDesc.SampleDesc.Quality = 0;
    depthDesc.Usage          = D3D11_USAGE_DEFAULT;
    depthDesc.BindFlags      = D3D11_BIND_DEPTH_STENCIL;

    m_device->CreateTexture2D(&depthDesc, nullptr, &m_depthStencilTexture);
    m_device->CreateDepthStencilView(m_depthStencilTexture.Get(), nullptr, &m_depthStencilView);
}

void DirectX11Renderer::DestroySwapChainResources() {
    m_renderTargetView.Reset();
    m_depthStencilView.Reset();
    m_depthStencilTexture.Reset();
}

// ==================================================================
// Shutdown
// ==================================================================
void DirectX11Renderer::Shutdown() {
    if (!m_initialized) return;

    m_activePipeline = nullptr;
    m_meshes.clear();
    m_pipelines.clear();
    m_textures.clear();
    m_buffers.clear();
    m_shaders.clear();

    m_defaultBlend.Reset();
    m_defaultDepthStencil.Reset();
    m_defaultRasterizer.Reset();
    DestroySwapChainResources();
    m_swapChain.Reset();

    if (m_context)  m_context->ClearState();
    m_context.Reset();
    m_device.Reset();
    m_initialized = false;
    LOG_INFO("DirectX 11 shut down");
}

// ==================================================================
// Frame control
// ==================================================================
void DirectX11Renderer::BeginFrame() {}
void DirectX11Renderer::EndFrame()   { FlushUniforms(); }
void DirectX11Renderer::Present()    { m_swapChain->Present(1, 0); }

void DirectX11Renderer::ClearColor(f32 r, f32 g, f32 b, f32 a) {
    FLOAT color[4] = { r, g, b, a };
    m_context->ClearRenderTargetView(m_renderTargetView.Get(), color);
    m_context->ClearDepthStencilView(m_depthStencilView.Get(),
                                     D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
}

void DirectX11Renderer::ClearDepth(f32 value) {
    m_context->ClearDepthStencilView(m_depthStencilView.Get(),
                                     D3D11_CLEAR_DEPTH, value, 0);
}

void DirectX11Renderer::SetViewport(const ViewportDesc& d) {
    D3D11_VIEWPORT vp = { d.x, d.y, d.width, d.height, d.minDepth, d.maxDepth };
    m_context->RSSetViewports(1, &vp);
}

// ==================================================================
// Create resources
// ==================================================================

ShaderHandle DirectX11Renderer::CreateShader(const ShaderDesc& desc) {
    D3D11Shader s;
    s.stage  = desc.stage;
    s.handle = (ShaderHandle)m_shaders.size();

    UINT compileFlags = D3DCOMPILE_OPTIMIZATION_LEVEL3;
#ifdef _DEBUG
    compileFlags |= D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif
    const char* target = (desc.stage == ShaderStage::Vertex) ? "vs_5_0" : "ps_5_0";

    Microsoft::WRL::ComPtr<ID3DBlob> error;
    HRESULT hr = D3DCompile(desc.source.c_str(), desc.source.size(),
                            nullptr, nullptr, nullptr,
                            desc.entryPoint.c_str(), target,
                            compileFlags, 0, &s.bytecode, &error);
    if (FAILED(hr)) {
        if (error) {
            LOG_ERROR("D3D shader compile error:\n{}",
                      (const char*)error->GetBufferPointer());
        } else {
            LOG_ERROR("D3D shader compile failed: {:08x}", hr);
        }
        return kInvalidHandle;
    }

    if (desc.stage == ShaderStage::Vertex) {
        hr = m_device->CreateVertexShader(s.bytecode->GetBufferPointer(),
                                           s.bytecode->GetBufferSize(),
                                           nullptr, &s.vs);
    } else {
        hr = m_device->CreatePixelShader(s.bytecode->GetBufferPointer(),
                                          s.bytecode->GetBufferSize(),
                                          nullptr, &s.ps);
    }
    if (FAILED(hr)) {
        LOG_ERROR("Create shader failed: {:08x}", hr);
        return kInvalidHandle;
    }

    m_shaders.push_back(std::move(s));
    return (ShaderHandle)(m_shaders.size() - 1);
}

BufferHandle DirectX11Renderer::CreateBuffer(const BufferDesc& desc) {
    D3D11Buffer b;
    b.type   = desc.type;
    b.size   = desc.size;
    b.stride = desc.stride;

    D3D11_BUFFER_DESC bd = {};
    bd.ByteWidth = desc.size;
    bd.StructureByteStride = desc.stride;

    switch (desc.type) {
        case BufferType::Vertex:   bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;   break;
        case BufferType::Index:    bd.BindFlags = D3D11_BIND_INDEX_BUFFER;    break;
        case BufferType::Constant: bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
                                   bd.ByteWidth = (desc.size + 15) & ~15;    // 16-byte aligned
                                   bd.Usage = D3D11_USAGE_DYNAMIC;
                                   bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
                                   break;
        default: break;
    }

    if (desc.type != BufferType::Constant) {
        bd.Usage = (desc.usage == ResourceUsage::Dynamic) ? D3D11_USAGE_DYNAMIC : D3D11_USAGE_DEFAULT;
        if (desc.usage == ResourceUsage::Dynamic)
            bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    }

    D3D11_SUBRESOURCE_DATA initData = {};
    initData.pSysMem = desc.initialData;

    HRESULT hr = m_device->CreateBuffer(&bd, desc.initialData ? &initData : nullptr, &b.buffer);
    if (FAILED(hr)) {
        LOG_ERROR("CreateBuffer failed: {:08x}", hr);
        return kInvalidHandle;
    }

    m_buffers.push_back(std::move(b));
    return (BufferHandle)(m_buffers.size() - 1);
}

TextureHandle DirectX11Renderer::CreateTexture(const TextureDesc& desc) {
    D3D11Texture t;
    t.width  = desc.width;
    t.height = desc.height;
    t.format = desc.format;

    // SRV
    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format              = ToDXGIFormat(desc.format, true);
    srvDesc.ViewDimension       = D3D11_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = desc.mipLevels;

    D3D11_TEXTURE2D_DESC texDesc = {};
    texDesc.Width          = desc.width;
    texDesc.Height         = desc.height;
    texDesc.MipLevels      = desc.mipLevels;
    texDesc.ArraySize      = 1;
    texDesc.Format         = ToDXGIFormat(desc.format);
    texDesc.SampleDesc.Count   = 1;
    texDesc.SampleDesc.Quality = 0;
    texDesc.Usage          = D3D11_USAGE_DEFAULT;
    texDesc.BindFlags      = D3D11_BIND_SHADER_RESOURCE;
    texDesc.MiscFlags      = (desc.mipLevels > 1) ? D3D11_RESOURCE_MISC_GENERATE_MIPS : 0;

    Microsoft::WRL::ComPtr<ID3D11Texture2D> tex;
    D3D11_SUBRESOURCE_DATA initDataDesc = {};
    initDataDesc.pSysMem     = desc.initialData;
    initDataDesc.SysMemPitch = desc.width * 4; // assume 4 bytes per pixel

    HRESULT hr = m_device->CreateTexture2D(
        &texDesc,
        desc.initialData ? &initDataDesc : nullptr,
        &tex);
    if (FAILED(hr)) { LOG_ERROR("CreateTexture2D failed: {:08x}", hr); return kInvalidHandle; }

    hr = m_device->CreateShaderResourceView(tex.Get(), &srvDesc, &t.srv);
    if (FAILED(hr)) { LOG_ERROR("CreateSRV failed: {:08x}", hr); return kInvalidHandle; }

    if (desc.mipLevels > 1)
        m_context->GenerateMips(t.srv.Get());

    // Sampler (linear)
    D3D11_SAMPLER_DESC sampDesc = {};
    sampDesc.Filter         = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    sampDesc.AddressU       = D3D11_TEXTURE_ADDRESS_WRAP;
    sampDesc.AddressV       = D3D11_TEXTURE_ADDRESS_WRAP;
    sampDesc.AddressW       = D3D11_TEXTURE_ADDRESS_WRAP;
    sampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
    sampDesc.MaxLOD         = D3D11_FLOAT32_MAX;
    m_device->CreateSamplerState(&sampDesc, &t.sampler);

    m_textures.push_back(std::move(t));
    return (TextureHandle)(m_textures.size() - 1);
}

PipelineHandle DirectX11Renderer::CreatePipelineState(const PipelineStateDesc& desc) {
    D3D11Pipeline p;
    p.topology = desc.topology;

    // --- Shaders ---
    // Vertex shader
    if (desc.vertexShader) {
        D3D11Shader tempVS;
        tempVS.stage = ShaderStage::Vertex;
        UINT flags = D3DCOMPILE_OPTIMIZATION_LEVEL3;
#ifdef _DEBUG
        flags |= D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif
        Microsoft::WRL::ComPtr<ID3DBlob> error;
        HRESULT hr = D3DCompile(desc.vertexShader->source.c_str(),
                                desc.vertexShader->source.size(),
                                nullptr, nullptr, nullptr,
                                desc.vertexShader->entryPoint.c_str(), "vs_5_0",
                                flags, 0, &tempVS.bytecode, &error);
        if (FAILED(hr)) {
            if (error) LOG_ERROR("VS compile error:\n{}", (const char*)error->GetBufferPointer());
            return kInvalidHandle;
        }
        m_device->CreateVertexShader(tempVS.bytecode->GetBufferPointer(),
                                     tempVS.bytecode->GetBufferSize(), nullptr, &p.vs);
        // Reflect
        u32 vsCBSize = 0;
        ReflectShaderConstants(tempVS.bytecode.Get(), p.vsSlots, vsCBSize);
        if (!p.vsSlots.empty()) {
            p.vsShadow.resize(MAX_CB_SIZE, 0);
            D3D11_BUFFER_DESC cbd = {};
            cbd.ByteWidth      = MAX_CB_SIZE;
            cbd.Usage          = D3D11_USAGE_DYNAMIC;
            cbd.BindFlags      = D3D11_BIND_CONSTANT_BUFFER;
            cbd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
            m_device->CreateBuffer(&cbd, nullptr, &p.vsCB);
        }
    }

    // Pixel shader
    if (desc.fragmentShader) {
        D3D11Shader tempPS;
        tempPS.stage = ShaderStage::Fragment;
        UINT flags = D3DCOMPILE_OPTIMIZATION_LEVEL3;
#ifdef _DEBUG
        flags |= D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif
        Microsoft::WRL::ComPtr<ID3DBlob> error;
        HRESULT hr = D3DCompile(desc.fragmentShader->source.c_str(),
                                desc.fragmentShader->source.size(),
                                nullptr, nullptr, nullptr,
                                desc.fragmentShader->entryPoint.c_str(), "ps_5_0",
                                flags, 0, &tempPS.bytecode, &error);
        if (FAILED(hr)) {
            if (error) LOG_ERROR("PS compile error:\n{}", (const char*)error->GetBufferPointer());
            return kInvalidHandle;
        }
        m_device->CreatePixelShader(tempPS.bytecode->GetBufferPointer(),
                                    tempPS.bytecode->GetBufferSize(), nullptr, &p.ps);
        // Reflect
        u32 psCBSize = 0;
        ReflectShaderConstants(tempPS.bytecode.Get(), p.psSlots, psCBSize);
        if (!p.psSlots.empty()) {
            p.psShadow.resize(MAX_CB_SIZE, 0);
            D3D11_BUFFER_DESC cbd = {};
            cbd.ByteWidth      = MAX_CB_SIZE;
            cbd.Usage          = D3D11_USAGE_DYNAMIC;
            cbd.BindFlags      = D3D11_BIND_CONSTANT_BUFFER;
            cbd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
            m_device->CreateBuffer(&cbd, nullptr, &p.psCB);
        }
    }

    // --- Input layout from vertex shader bytecode ---
    if (desc.vertexShader) {
        // We need the VS bytecode — for simplicity, re-compile to get bytecode.
        // In a production system you'd store the bytecode on the shader side.
        // Since the shader bytecode is already inside D3D11Shader from CreateShader,
        // we create a temporary here. A more efficient approach would store bytecode
        // per shader handle.
        Microsoft::WRL::ComPtr<ID3DBlob> vsBytecode;
        UINT flags = D3DCOMPILE_OPTIMIZATION_LEVEL3;
        Microsoft::WRL::ComPtr<ID3DBlob> error;
        D3DCompile(desc.vertexShader->source.c_str(),
                   desc.vertexShader->source.size(),
                   nullptr, nullptr, nullptr,
                   desc.vertexShader->entryPoint.c_str(), "vs_5_0",
                   flags, 0, &vsBytecode, &error);

        if (vsBytecode) {
            std::vector<D3D11_INPUT_ELEMENT_DESC> elements;
            for (size_t i = 0; i < desc.vertexLayout.size(); ++i) {
                const auto& e = desc.vertexLayout[i];
                D3D11_INPUT_ELEMENT_DESC ie = {};
                ie.SemanticName         = e.semanticName.c_str();
                ie.SemanticIndex        = 0;
                ie.Format               = ToDXGIFormat(e.format);
                ie.InputSlot            = e.slot;
                ie.AlignedByteOffset    = e.offset;
                ie.InputSlotClass       = (D3D11_INPUT_CLASSIFICATION)e.inputSlotClass;
                elements.push_back(ie);
            }
            m_device->CreateInputLayout(elements.data(), (UINT)elements.size(),
                                        vsBytecode->GetBufferPointer(),
                                        vsBytecode->GetBufferSize(),
                                        &p.inputLayout);
        }
    }

    // --- Rasterizer state ---
    {
        D3D11_RASTERIZER_DESC rd = {};
        rd.FillMode              = ToD3DFillMode(desc.fillMode);
        rd.CullMode              = ToD3DCullMode(desc.cullMode);
        rd.FrontCounterClockwise = FALSE;
        rd.DepthClipEnable       = TRUE;
        rd.ScissorEnable         = FALSE;
        rd.MultisampleEnable     = FALSE;
        rd.AntialiasedLineEnable = FALSE;
        m_device->CreateRasterizerState(&rd, &p.rasterizer);
    }

    // --- Depth-stencil state ---
    {
        D3D11_DEPTH_STENCIL_DESC dd = {};
        dd.DepthEnable      = desc.depthTestEnabled ? TRUE : FALSE;
        dd.DepthWriteMask   = desc.depthWriteEnabled ? D3D11_DEPTH_WRITE_MASK_ALL : D3D11_DEPTH_WRITE_MASK_ZERO;
        dd.DepthFunc        = ToD3DCompare(desc.depthFunc);
        dd.StencilEnable    = FALSE;
        m_device->CreateDepthStencilState(&dd, &p.depthStencil);
    }

    // --- Blend state ---
    {
        D3D11_BLEND_DESC bd = {};
        bd.AlphaToCoverageEnable  = FALSE;
        bd.IndependentBlendEnable = FALSE;
        D3D11_RENDER_TARGET_BLEND_DESC rt = {};
        rt.BlendEnable = (desc.blendMode != BlendMode::Opaque) ? TRUE : FALSE;
        switch (desc.blendMode) {
            case BlendMode::Opaque:
                rt.SrcBlend = D3D11_BLEND_ONE; rt.DestBlend = D3D11_BLEND_ZERO;
                rt.SrcBlendAlpha = D3D11_BLEND_ONE; rt.DestBlendAlpha = D3D11_BLEND_ZERO;
                break;
            case BlendMode::AlphaBlend:
                rt.SrcBlend = D3D11_BLEND_SRC_ALPHA; rt.DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
                rt.SrcBlendAlpha = D3D11_BLEND_ONE; rt.DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
                break;
            case BlendMode::Additive:
                rt.SrcBlend = D3D11_BLEND_ONE; rt.DestBlend = D3D11_BLEND_ONE;
                rt.SrcBlendAlpha = D3D11_BLEND_ONE; rt.DestBlendAlpha = D3D11_BLEND_ONE;
                break;
            case BlendMode::Multiply:
                rt.SrcBlend = D3D11_BLEND_ZERO; rt.DestBlend = D3D11_BLEND_SRC_COLOR;
                rt.SrcBlendAlpha = D3D11_BLEND_ZERO; rt.DestBlendAlpha = D3D11_BLEND_SRC_ALPHA;
                break;
        }
        rt.BlendOp               = D3D11_BLEND_OP_ADD;
        rt.BlendOpAlpha          = D3D11_BLEND_OP_ADD;
        rt.RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
        bd.RenderTarget[0] = rt;
        m_device->CreateBlendState(&bd, &p.blendState);
    }

    m_pipelines.push_back(std::move(p));
    return (PipelineHandle)(m_pipelines.size() - 1);
}

MeshHandle DirectX11Renderer::CreateMesh(const MeshData& data) {
    D3D11Mesh m;
    m.vertexCount  = data.vertexCount;
    m.indexCount   = data.indexCount;
    m.vertexStride = data.vertexStride;

    // Vertex buffer
    if (!data.vertices.empty()) {
        D3D11_BUFFER_DESC vbd = {};
        vbd.ByteWidth      = (UINT)(data.vertices.size() * sizeof(f32));
        vbd.Usage          = D3D11_USAGE_IMMUTABLE;
        vbd.BindFlags      = D3D11_BIND_VERTEX_BUFFER;
        vbd.StructureByteStride = data.vertexStride;
        D3D11_SUBRESOURCE_DATA vsd = {};
        vsd.pSysMem = data.vertices.data();
        m_device->CreateBuffer(&vbd, &vsd, &m.vertexBuffer);
    }

    // Index buffer
    if (!data.indices.empty()) {
        D3D11_BUFFER_DESC ibd = {};
        ibd.ByteWidth      = (UINT)(data.indices.size() * sizeof(u32));
        ibd.Usage          = D3D11_USAGE_IMMUTABLE;
        ibd.BindFlags      = D3D11_BIND_INDEX_BUFFER;
        D3D11_SUBRESOURCE_DATA isd = {};
        isd.pSysMem = data.indices.data();
        m_device->CreateBuffer(&ibd, &isd, &m.indexBuffer);
    }

    m_meshes.push_back(std::move(m));
    return (MeshHandle)(m_meshes.size() - 1);
}

// ==================================================================
// Destroy resources
// ==================================================================
void DirectX11Renderer::DestroyShader(ShaderHandle h) {
    if (h < m_shaders.size()) {
        m_shaders[h] = D3D11Shader{}; // ComPtr resets automatically
    }
}
void DirectX11Renderer::DestroyBuffer(BufferHandle h) {
    if (h < m_buffers.size()) {
        m_buffers[h] = D3D11Buffer{};
    }
}
void DirectX11Renderer::DestroyTexture(TextureHandle h) {
    if (h < m_textures.size()) {
        m_textures[h] = D3D11Texture{};
    }
}
void DirectX11Renderer::DestroyPipelineState(PipelineHandle h) {
    if (h < m_pipelines.size()) {
        m_pipelines[h] = D3D11Pipeline{};
    }
}
void DirectX11Renderer::DestroyMesh(MeshHandle h) {
    if (h < m_meshes.size()) {
        m_meshes[h] = D3D11Mesh{};
    }
}

// ==================================================================
// Binding
// ==================================================================
void DirectX11Renderer::BindPipelineState(PipelineHandle h) {
    if (h >= m_pipelines.size()) return;
    m_activePipeline = &m_pipelines[h];

    // Shaders
    m_context->VSSetShader(m_activePipeline->vs.Get(), nullptr, 0);
    m_context->PSSetShader(m_activePipeline->ps.Get(), nullptr, 0);

    // Input layout
    m_context->IASetInputLayout(m_activePipeline->inputLayout.Get());
    m_context->IASetPrimitiveTopology(ToD3DTopology(m_activePipeline->topology));

    // Fixed-function state
    m_context->RSSetState(m_activePipeline->rasterizer.Get()
                          ? m_activePipeline->rasterizer.Get()
                          : m_defaultRasterizer.Get());
    m_context->OMSetDepthStencilState(m_activePipeline->depthStencil.Get()
                                      ? m_activePipeline->depthStencil.Get()
                                      : m_defaultDepthStencil.Get(), 0);
    m_context->OMSetBlendState(m_activePipeline->blendState.Get()
                               ? m_activePipeline->blendState.Get()
                               : m_defaultBlend.Get(), nullptr, 0xFFFFFFFF);

    // Bind constant buffers
    if (m_activePipeline->vsCB)
        m_context->VSSetConstantBuffers(0, 1, m_activePipeline->vsCB.GetAddressOf());
    if (m_activePipeline->psCB)
        m_context->PSSetConstantBuffers(0, 1, m_activePipeline->psCB.GetAddressOf());
}

void DirectX11Renderer::BindVertexBuffer(BufferHandle handle, u32 slot) {
    if (handle >= m_buffers.size()) return;
    ID3D11Buffer* buf = m_buffers[handle].buffer.Get();
    UINT stride = m_buffers[handle].stride;
    UINT offset = 0;
    m_context->IASetVertexBuffers(slot, 1, &buf, &stride, &offset);
}

void DirectX11Renderer::BindIndexBuffer(BufferHandle handle) {
    if (handle >= m_buffers.size()) return;
    m_context->IASetIndexBuffer(m_buffers[handle].buffer.Get(),
                                DXGI_FORMAT_R32_UINT, 0);
}

void DirectX11Renderer::BindTexture(TextureHandle handle, u32 slot) {
    if (handle >= m_textures.size()) return;
    ID3D11ShaderResourceView* srv = m_textures[handle].srv.Get();
    ID3D11SamplerState* samp = m_textures[handle].sampler.Get();
    if (srv) {
        m_context->PSSetShaderResources(slot, 1, &srv);
        if (samp) m_context->PSSetSamplers(slot, 1, &samp);
    }
}

void DirectX11Renderer::BindMesh(MeshHandle handle) {
    if (handle >= m_meshes.size()) return;
    auto& m = m_meshes[handle];
    ID3D11Buffer* vb = m.vertexBuffer.Get();
    UINT stride = m.vertexStride;
    UINT offset = 0;
    m_context->IASetVertexBuffers(0, 1, &vb, &stride, &offset);
    if (m.indexBuffer) {
        m_context->IASetIndexBuffer(m.indexBuffer.Get(), m.indexFormat, 0);
    }
}

// ==================================================================
// Uniform setting (by-name via shadow-buffer reflection)
// ==================================================================
void DirectX11Renderer::FlushUniforms() {
    if (!m_activePipeline) return;

    if (m_activePipeline->vsDirty && m_activePipeline->vsCB) {
        D3D11_MAPPED_SUBRESOURCE mapped;
        HRESULT hr = m_context->Map(m_activePipeline->vsCB.Get(), 0,
                                     D3D11_MAP_WRITE_DISCARD, 0, &mapped);
        if (SUCCEEDED(hr)) {
            memcpy(mapped.pData, m_activePipeline->vsShadow.data(),
                   m_activePipeline->vsShadow.size());
            m_context->Unmap(m_activePipeline->vsCB.Get(), 0);
        }
        m_activePipeline->vsDirty = false;
    }

    if (m_activePipeline->psDirty && m_activePipeline->psCB) {
        D3D11_MAPPED_SUBRESOURCE mapped;
        HRESULT hr = m_context->Map(m_activePipeline->psCB.Get(), 0,
                                     D3D11_MAP_WRITE_DISCARD, 0, &mapped);
        if (SUCCEEDED(hr)) {
            memcpy(mapped.pData, m_activePipeline->psShadow.data(),
                   m_activePipeline->psShadow.size());
            m_context->Unmap(m_activePipeline->psCB.Get(), 0);
        }
        m_activePipeline->psDirty = false;
    }
}

static bool FindUniformSlot(const std::vector<D3D11UniformSlot>& slots,
                            const char* name, u32& outOffset, u32& outSize)
{
    for (const auto& s : slots) {
        if (s.name == name) {
            outOffset = s.offset;
            outSize   = s.size;
            return true;
        }
    }
    return false;
}

void DirectX11Renderer::SetUniformBuffer(BufferHandle handle, u32 slot) {
    if (handle >= m_buffers.size() || !m_buffers[handle].buffer) return;
    ID3D11Buffer* buf = m_buffers[handle].buffer.Get();
    m_context->VSSetConstantBuffers(slot, 1, &buf);
    m_context->PSSetConstantBuffers(slot, 1, &buf);
}

void DirectX11Renderer::SetUniformMat4(const char* name, const Mat4& matrix) {
    if (!m_activePipeline) return;

    u32 offset, size;
    if (FindUniformSlot(m_activePipeline->vsSlots, name, offset, size)) {
        if (offset + sizeof(glm::mat4) <= m_activePipeline->vsShadow.size()) {
            memcpy(&m_activePipeline->vsShadow[offset], glm::value_ptr(matrix), sizeof(glm::mat4));
            m_activePipeline->vsDirty = true;
        }
    }
    if (FindUniformSlot(m_activePipeline->psSlots, name, offset, size)) {
        if (offset + sizeof(glm::mat4) <= m_activePipeline->psShadow.size()) {
            memcpy(&m_activePipeline->psShadow[offset], glm::value_ptr(matrix), sizeof(glm::mat4));
            m_activePipeline->psDirty = true;
        }
    }
}

void DirectX11Renderer::SetUniformVec3(const char* name, const Vec3& value) {
    if (!m_activePipeline) return;

    u32 offset, size;
    if (FindUniformSlot(m_activePipeline->vsSlots, name, offset, size)) {
        if (offset + sizeof(glm::vec3) <= m_activePipeline->vsShadow.size()) {
            memcpy(&m_activePipeline->vsShadow[offset], glm::value_ptr(value), sizeof(glm::vec3));
            m_activePipeline->vsDirty = true;
        }
    }
    if (FindUniformSlot(m_activePipeline->psSlots, name, offset, size)) {
        if (offset + sizeof(glm::vec3) <= m_activePipeline->psShadow.size()) {
            memcpy(&m_activePipeline->psShadow[offset], glm::value_ptr(value), sizeof(glm::vec3));
            m_activePipeline->psDirty = true;
        }
    }
}

void DirectX11Renderer::SetUniformVec4(const char* name, const Vec4& value) {
    if (!m_activePipeline) return;

    u32 offset, size;
    if (FindUniformSlot(m_activePipeline->vsSlots, name, offset, size)) {
        if (offset + sizeof(glm::vec4) <= m_activePipeline->vsShadow.size()) {
            memcpy(&m_activePipeline->vsShadow[offset], glm::value_ptr(value), sizeof(glm::vec4));
            m_activePipeline->vsDirty = true;
        }
    }
    if (FindUniformSlot(m_activePipeline->psSlots, name, offset, size)) {
        if (offset + sizeof(glm::vec4) <= m_activePipeline->psShadow.size()) {
            memcpy(&m_activePipeline->psShadow[offset], glm::value_ptr(value), sizeof(glm::vec4));
            m_activePipeline->psDirty = true;
        }
    }
}

void DirectX11Renderer::SetUniformFloat(const char* name, f32 value) {
    if (!m_activePipeline) return;

    u32 offset, size;
    if (FindUniformSlot(m_activePipeline->vsSlots, name, offset, size)) {
        if (offset + sizeof(f32) <= m_activePipeline->vsShadow.size()) {
            memcpy(&m_activePipeline->vsShadow[offset], &value, sizeof(f32));
            m_activePipeline->vsDirty = true;
        }
    }
    if (FindUniformSlot(m_activePipeline->psSlots, name, offset, size)) {
        if (offset + sizeof(f32) <= m_activePipeline->psShadow.size()) {
            memcpy(&m_activePipeline->psShadow[offset], &value, sizeof(f32));
            m_activePipeline->psDirty = true;
        }
    }
}

void DirectX11Renderer::SetUniformInt(const char* name, i32 value) {
    if (!m_activePipeline) return;

    u32 offset, size;
    if (FindUniformSlot(m_activePipeline->vsSlots, name, offset, size)) {
        if (offset + sizeof(i32) <= m_activePipeline->vsShadow.size()) {
            memcpy(&m_activePipeline->vsShadow[offset], &value, sizeof(i32));
            m_activePipeline->vsDirty = true;
        }
    }
    if (FindUniformSlot(m_activePipeline->psSlots, name, offset, size)) {
        if (offset + sizeof(i32) <= m_activePipeline->psShadow.size()) {
            memcpy(&m_activePipeline->psShadow[offset], &value, sizeof(i32));
            m_activePipeline->psDirty = true;
        }
    }
}

// ==================================================================
// Draw
// ==================================================================
void DirectX11Renderer::Draw(u32 vertexCount, u32 startVertex) {
    FlushUniforms();
    m_context->Draw((UINT)vertexCount, (UINT)startVertex);
}

void DirectX11Renderer::DrawIndexed(u32 indexCount, u32 startIndex, i32 baseVertex) {
    FlushUniforms();
    m_context->DrawIndexed((UINT)indexCount, (UINT)startIndex, (INT)baseVertex);
}

void DirectX11Renderer::DrawMesh(MeshHandle handle) {
    if (handle >= m_meshes.size()) return;
    FlushUniforms();

    auto& m = m_meshes[handle];
    ID3D11Buffer* vb = m.vertexBuffer.Get();
    UINT stride = m.vertexStride;
    UINT offset = 0;
    m_context->IASetVertexBuffers(0, 1, &vb, &stride, &offset);

    if (m.indexCount > 0) {
        m_context->IASetIndexBuffer(m.indexBuffer.Get(), m.indexFormat, 0);
        m_context->DrawIndexed((UINT)m.indexCount, 0, 0);
    } else {
        m_context->IASetIndexBuffer(nullptr, DXGI_FORMAT_UNKNOWN, 0);
        m_context->Draw((UINT)m.vertexCount, 0);
    }
}

// ==================================================================
// Resize
// ==================================================================
void DirectX11Renderer::OnResize(u32 width, u32 height) {
    if (width == 0 || height == 0) return;
    m_width  = width;
    m_height = height;

    m_context->OMSetRenderTargets(0, nullptr, nullptr);
    DestroySwapChainResources();

    HRESULT hr = m_swapChain->ResizeBuffers(0, width, height,
                                             DXGI_FORMAT_UNKNOWN, 0);
    if (FAILED(hr)) {
        LOG_ERROR("ResizeBuffers failed: {:08x}", hr);
        return;
    }

    CreateSwapChainResources(m_hwnd);
    m_context->OMSetRenderTargets(1, m_renderTargetView.GetAddressOf(),
                                   m_depthStencilView.Get());

    D3D11_VIEWPORT vp = { 0, 0, (FLOAT)width, (FLOAT)height, 0.0f, 1.0f };
    m_context->RSSetViewports(1, &vp);
}

} // namespace painkiller
