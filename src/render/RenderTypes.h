#pragma once

#include "Types.h"
#include "Vector.h"
#include "Matrix.h"
#include <string>
#include <vector>

namespace nebula {

enum class PrimitiveTopology : u8 {
    TriangleList, TriangleStrip, LineList, LineStrip, PointList,
};
enum class CompareFunction : u8 {
    Never, Less, Equal, LessEqual, Greater, NotEqual, GreaterEqual, Always,
};
enum class CullMode : u8 { None, Front, Back, };
enum class BlendMode : u8 { Opaque, AlphaBlend, Additive, Multiply, };
enum class FillMode : u8 { Solid, Wireframe, };
enum class Format : u8 {
    R8G8B8A8_UNORM, R8G8B8A8_SRGB, R32G32B32A32_FLOAT, R32G32B32_FLOAT, R32G32_FLOAT, R32_FLOAT, D32_FLOAT, D24_UNORM_S8_UINT, Unknown,
};
enum class AddressMode : u8 { Repeat, Clamp, Mirror, Border, };
enum class FilterMode : u8 { Point, Linear, Anisotropic, };
enum class LightType : u8 { Directional, Point, Spot, };

struct ShaderDesc { ShaderStage stage; std::string source; std::string entryPoint = "main"; };
struct BufferDesc { BufferType type; ResourceUsage usage = ResourceUsage::Static; u32 size = 0; u32 stride = 0; void* initialData = nullptr; };
struct TextureDesc { TextureType type = TextureType::Texture2D; u32 width = 0; u32 height = 0; u32 depth = 1; u32 mipLevels = 1; Format format = Format::R8G8B8A8_UNORM; void* initialData = nullptr; };
struct VertexInputElement { std::string semanticName; Format format; u32 offset = 0; u32 slot = 0; u32 inputSlotClass = 0; };
struct PipelineStateDesc {
    ShaderDesc* vertexShader = nullptr; ShaderDesc* fragmentShader = nullptr;
    PrimitiveTopology topology = PrimitiveTopology::TriangleList;
    CullMode cullMode = CullMode::Back; FillMode fillMode = FillMode::Solid;
    BlendMode blendMode = BlendMode::Opaque;
    bool depthTestEnabled = true; bool depthWriteEnabled = true;
    CompareFunction depthFunc = CompareFunction::Less;
    std::vector<VertexInputElement> vertexLayout;
    u32 numRenderTargets = 1;
};
struct ViewportDesc { f32 x = 0, y = 0, width = 0, height = 0, minDepth = 0, maxDepth = 1; };

struct LightData { LightType type = LightType::Directional; Vec3 color = Vec3(1.0f); f32 intensity = 1.0f; f32 range = 10.0f; f32 spotAngle = 45.0f; };

struct MeshData {
    std::vector<f32> vertices;
    std::vector<u32> indices;
    u32 vertexStride = 0, vertexCount = 0, indexCount = 0;
    static MeshData CreateCube(f32 size = 1.0f);
    static MeshData CreateSphere(f32 radius = 0.5f, u32 segments = 32, u32 rings = 16);
    static MeshData CreatePlane(f32 width = 1.0f, f32 depth = 1.0f);
    static MeshData CreateTriangle();
};

} // namespace nebula
