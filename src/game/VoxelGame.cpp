#include "VoxelGame.h"
#include "Logger.h"
#include "Math.h"
#include "BlockRaycast.h"
#include "TextureLoader.h"
#include "ResourceManager.h"
#include <cstdlib>
#include <algorithm>
#include <cstdio>
#include <cmath>

namespace painkiller {

// ============================================================
// Embedded World Shaders
// ============================================================
static const char* kWorldVertexSrc = R"(
    #version 330 core
    layout(location = 0) in vec3 a_Position;
    layout(location = 1) in vec3 a_Normal;
    layout(location = 2) in vec2 a_TexCoord;

    uniform mat4 u_View;
    uniform mat4 u_Projection;
    uniform vec3 u_CameraPos;

    out vec3 v_Normal;
    out vec3 v_FragPos;
    out vec3 v_ViewDir;
    out vec2 v_TexCoord;

    void main() {
        vec4 worldPos = vec4(a_Position, 1.0);
        gl_Position = u_Projection * u_View * worldPos;
        v_Normal   = a_Normal;
        v_FragPos  = a_Position;
        v_ViewDir  = normalize(u_CameraPos - a_Position);
        v_TexCoord = a_TexCoord;
    }
)";

static const char* kWorldFragmentSrc = R"(
    #version 330 core
    in vec3 v_Normal;
    in vec3 v_FragPos;
    in vec3 v_ViewDir;
    in vec2 v_TexCoord;

    uniform vec3 u_LightDir;
    uniform vec3 u_LightColor;
    uniform float u_Time;
    uniform sampler2D u_BlockAtlas;
    // Atlas grid: u_AtlasGrid.x = cols (4), u_AtlasGrid.y = rows (3)
    uniform float u_AtlasGridX;
    uniform float u_AtlasGridY;

    out vec4 FragColor;

    int getBlockType() { return int(v_TexCoord.x * 255.0 + 0.5); }
    int getFace()      { return int(v_TexCoord.y * 6.0 + 0.5); }

    // Get atlas cell UV from block type and face
    // Returns cell index (0-11) and signal for procedural fallback
    int getAtlasCell(int type, int face) {
        if (type == 1) {  // Grass
            if (face == 2) return 0;       // top: grass_top
            else if (face == 3) return 2;  // bottom: dirt
            else return 1;                 // sides: grass_side
        }
        if (type == 2) return 2;           // Dirt
        if (type == 3) return 3;           // Stone
        if (type == 4) return 4;           // Cobblestone
        if (type == 5) return -1;          // Wood (unused) - procedural
        if (type == 6) {                   // OakLog (CORRECTED: was type 5)
            if (face == 2 || face == 3) return 5; // top/bottom: oak_log_top
            else return 6;                       // sides: oak_log
        }
        if (type == 7) return -1;          // Leaves - procedural
        if (type == 8) return 2;           // OakPlanks -> dirt fallback
        if (type == 9) return -2;          // Water - transparent
        if (type == 10) return 8;          // Snow
        if (type == 11) return -2;         // Water (duplicate)
        if (type == 12) return 9;          // Bedrock
        return -1;
    }

    // Compute face-local UV from fragment world position
    vec2 getFaceUV(vec3 pos, int f) {
        vec3 l = fract(pos);
        if (f == 0) return vec2(1.0 - l.z, l.y);       // +X Right
        else if (f == 1) return vec2(l.z, l.y);         // -X Left
        else if (f == 2) return vec2(l.x, 1.0 - l.z);     // +Y Top
        else if (f == 3) return vec2(l.x, l.z);            // -Y Bottom
        else if (f == 4) return vec2(1.0 - l.x, l.y);    // +Z Front
        else return vec2(l.x, l.y);                      // -Z Back
    }

    // Water animation wave
    float waterWave(vec3 pos, float t) {
        return 0.02 * sin(pos.x * 2.0 + pos.z * 3.0 + t * 4.0)
             + 0.015 * sin(pos.x * 3.5 + pos.z * 1.7 + t * 3.0);
    }

    void main() {
        vec3 normal = normalize(v_Normal);
        vec3 lightDir = normalize(u_LightDir);
        // Hemispheric ambient: sky blue from above, warm brown from below
        float hemi_top = max(normal.y, 0.0);
        float hemi_bot = max(-normal.y, 0.0);
        vec3 ambient = vec3(0.35, 0.40, 0.50) * hemi_top + vec3(0.15, 0.10, 0.08) * hemi_bot + vec3(0.15, 0.15, 0.18);
        float diff = max(dot(normal, -lightDir), 0.0);
        vec3 diffuse = diff * u_LightColor;
        vec3 viewDir = normalize(v_ViewDir);
        vec3 halfDir = normalize(-lightDir + viewDir);
        float spec = pow(max(dot(normal, halfDir), 0.0), 8.0);
        vec3 specular = spec * u_LightColor * 0.1;
        
        int bt = getBlockType();
        int face = getFace();
        int cell = getAtlasCell(bt, face);
        vec2 uv = getFaceUV(v_FragPos, face);
        vec3 color;
        float alpha = 1.0;
        
        if (cell >= 0) {
            // Sample from atlas cell using face-local UV
            float cols = u_AtlasGridX;
            float rows = u_AtlasGridY;
            float cellCol = mod(float(cell), cols);
            float cellRow = floor(float(cell) / cols);
            // stb_image flips Y when loading, so row 0 is at bottom of texture data
            vec2 atlasUV = vec2((cellCol + uv.x) / cols, ((rows - 1.0 - cellRow) + uv.y) / rows);
            vec4 texColor = texture(u_BlockAtlas, atlasUV);
            color = texColor.rgb;
        } else if (cell == -2) {
            // Water - animated semi-transparent blue
            float wave = waterWave(v_FragPos, u_Time);
            vec3 wc = vec3(0.15, 0.35, 0.75);
            vec3 fc = vec3(0.3, 0.6, 0.9);
            color = mix(wc, fc, wave * 2.0 + 0.5);
            alpha = 0.6;
        } else {
            // Leaves: semi-transparent with dither pattern for see-through foliage
            float leaf_dither = fract(v_FragPos.x * 3.0 + v_FragPos.y * 5.0 + v_FragPos.z * 7.0);
            if (leaf_dither < 0.20) discard; // 20% holes for transparency
            float f = 0.20 + 0.06 * sin(v_FragPos.x * 2.5 + v_FragPos.z * 2.0);
            color = vec3(f, 0.55, f * 0.4);
            alpha = 0.85;
        }
        
        vec3 result = (ambient + diffuse + specular) * color;
        float dist = length(v_FragPos);
        float fog = clamp((dist - 50.0) / 40.0, 0.0, 0.45);
        vec3 fogColor = vec3(0.55, 0.75, 0.95);
        FragColor.rgb = mix(result, fogColor, fog);
        FragColor.a = alpha;
    }
)";

// ============================================================
// Embedded Skybox Shaders (procedural gradient)
// ============================================================
static const char* kSkyboxVertexSrc = R"(
    #version 330 core
    layout(location = 0) in vec3 a_Position;
    uniform mat4 u_Projection;
    uniform mat4 u_View;
    out vec3 v_Dir;
    void main() {
        vec4 pos = u_Projection * u_View * vec4(a_Position, 1.0);
        gl_Position = pos.xyww;
        v_Dir = a_Position;
    }
)";

static const char* kSkyboxFragmentSrc = R"(
    #version 330 core
    in vec3 v_Dir;
    out vec4 FragColor;
    void main() {
        vec3 dir = normalize(v_Dir);
        float h = dir.y;
        vec3 sky = vec3(0.35, 0.60, 0.90);
        vec3 horizon = vec3(0.60, 0.78, 0.95);
        vec3 ground = vec3(0.50, 0.42, 0.32);
        vec3 color = mix(ground, horizon, smoothstep(-0.1, 0.0, h));
        color = mix(color, sky, smoothstep(0.0, 0.4, h));
        FragColor = vec4(color, 1.0);
    }
)";

// ============================================================
// VoxelGame Implementation
// ============================================================
VoxelGame::VoxelGame() {}
VoxelGame::~VoxelGame() { Shutdown(nullptr); }

bool VoxelGame::Initialize(Engine* engine) {
    LOG_INFO("VoxelGame initializing...");
    auto* renderer = engine->GetRenderer();
    auto* scene = engine->GetScene();

    // 1. Create world
    m_world = new World(renderer);
    m_world->Initialize(m_seed);

    // 2. Setup shaders
    SetupShadersAndPipeline(renderer);

    // 3. Init player
    m_player.Initialize(scene->GetActiveCamera());
    m_player.SetWorld(m_world);

    // Adjust player spawn to be above terrain surface
    i32 terrainHeight = m_world->GetTerrainGenerator().GetHeightAt(0, 0);
    m_player.SetPosition(Vec3(0.0f, (f32)(terrainHeight + 3), 0.0f));

    // 4. Particle system
    m_particleSystem.Initialize(renderer);

    // 5. Sound
    m_soundManager.Initialize();

    // 6. Load initial chunks
    UpdateChunks(engine);

                        
// 7. Generate procedural textures
    {
        const int kSize = 16; const int kCols = 4, kRows = 3;
        int atlasW = kCols * kSize, atlasH = kRows * kSize;
        std::vector<u8> atlas(atlasW * atlasH * 4, (u8)255);
        auto hash = [](int x, int y, int seed) -> float {
            unsigned int h = (unsigned int)(x * 374761393u + y * 668265263u + seed * 1274126177u);
            h = (h ^ (h >> 13)) * 1274126177u; h = h ^ (h >> 16);
            return (float)(h & 0xFF) / 255.0f;
        };
        auto sn = [&](int x, int y, int seed) -> float {
            int gx = x / 4, gy = y / 4;
            float fx = (float)(x % 4) / 4.0f, fy = (float)(y % 4) / 4.0f;
            return (hash(gx,gy,seed)*(1-fx)+hash(gx+1,gy,seed)*fx)*(1-fy)+
                   (hash(gx,gy+1,seed)*(1-fx)+hash(gx+1,gy+1,seed)*fx)*fy;
        };
        auto cb = [](float v) -> u8 { return (u8)std::max(0.0f, std::min(255.0f, v*255.0f)); };
        auto sp = [&](int ci, int px, int py, u8 r, u8 g, u8 b) {
            int cx = ci % kCols, cr = ci / kCols;
            int dx = cx * kSize + px, dy = (kRows-1-cr)*kSize + (kSize-1-py);
            int idx = (dy * atlasW + dx) * 4;
            atlas[idx]=r; atlas[idx+1]=g; atlas[idx+2]=b; atlas[idx+3]=255;
        };
        for (int c = 0; c < 10; c++)
        for (int py = 0; py < kSize; py++)
        for (int px = 0; px < kSize; px++) {
            float n = sn(px, py, c+100), n2 = sn(px+3, py+7, c+200);
            float r=0, g=0, b=0;
            if (c == 0) { r=0.15f+n*0.20f; g=0.48f+n2*0.27f; b=0.08f+n*0.10f;
                float t=sn(px/2,py/2,c+300); if(t<0.25f) { r+=0.05f; g-=0.10f; } if(t>0.7f) { g+=0.08f; r-=0.03f; } }
            else if (c == 1) { // Grass Side
                if (py<6) { r=0.15f+n*0.20f; g=0.48f+n2*0.25f; b=0.08f+n*0.10f; }
                else if (py<10) { float t=(py-6)/4.0f;
                    r=(0.15f+n*0.20f)*(1-t)+(0.45f+n*0.15f)*t;
                    g=(0.48f+n2*0.25f)*(1-t)+(0.30f+n2*0.10f)*t;
                    b=(0.08f+n*0.10f)*(1-t)+(0.15f+n*0.08f)*t; }
                else { r=0.45f+n*0.15f; g=0.30f+n2*0.10f; b=0.15f+n*0.08f; } }
            else if (c == 2) { float sp2=sn(px*3,py*3,c+50);
                r=0.40f+n*0.18f+(sp2>0.7f?0.08f:0); g=0.28f+n2*0.12f+(sp2>0.7f?0.05f:0); b=0.12f+n*0.08f; }
            else if (c == 3) { float cr=sn(px*2+1,py*2+1,c+33); float v=0.50f+n*0.15f;
                if(cr>0.7f)v-=0.20f; if(sn(px*3,py*3,c+44)>0.85f)v+=0.12f; v=std::max(0.25f,std::min(0.75f,v)); r=g=b=v; }
            else if (c == 4) { int cx=px%8-4,cy=py%8-4; float d=(float)(cx*cx+cy*cy)/16.0f;
                if(d>0.8f||(abs(px-8)<2&&abs(py-8)<2)) { r=0.20f+n*0.10f; g=r; b=r; }
                else { float v=0.50f+n*0.25f; r=g=b=v; } }
            else if (c == 5) { float dx=(float)px-7.5f,dy=(float)py-7.5f;
                float dist=sqrtf(dx*dx+dy*dy); float ring=fmodf(dist+n*0.5f,3.0f)/3.0f;
                float base=0.45f+n*0.15f; if(ring<0.25f)base-=0.12f; else if(ring>0.7f)base-=0.05f;
                r=(0.50f+n2*0.10f)*base/0.5f; g=(0.35f+n2*0.08f)*base/0.5f; b=(0.15f+n2*0.05f)*base/0.5f; }
            else if (c == 6) { float stripe=(float)(px%4)/4.0f; float base=0.40f+n*0.12f;
                if(stripe<0.25f)base-=0.10f; else if(stripe>0.75f)base-=0.05f;
                if(sn(px/2,py,c+77)>0.6f)base+=0.05f;
                r=(0.45f+n2*0.10f)*base/0.45f; g=(0.30f+n2*0.08f)*base/0.45f; b=(0.15f+n2*0.05f)*base/0.45f; }
            else if (c == 7) { float sp2=sn(px*5,py*5,c+200);
                r=0.68f+n*0.15f+(sp2>0.8f?0.06f:0); g=0.62f+n2*0.15f+(sp2>0.8f?0.06f:0); b=0.40f+n*0.12f+(sp2>0.8f?0.03f:0); }
            else if (c == 8) { float v=0.92f+n*0.06f; if(sn(px*3,py*3,c+60)<0.3f)v-=0.05f; r=v; g=v; b=v-0.02f+n2*0.03f; }
            else if (c == 9) { float cr=sn(px*2+3,py*2+3,c+99); float v=0.18f+n*0.18f;
                if(cr>0.6f)v-=0.10f; if(sn(px*4+1,py*4+1,c+55)>0.85f)v+=0.12f;
                v=std::max(0.08f,std::min(0.45f,v)); r=g=b=v; }
            sp(c, px, py, cb(r), cb(g), cb(b));
        }
        TextureDesc atlasDesc;
        atlasDesc.type = TextureType::Texture2D;
        atlasDesc.width = (u32)atlasW; atlasDesc.height = (u32)atlasH;
        atlasDesc.depth = 1; atlasDesc.mipLevels = 1;
        atlasDesc.format = Format::R8G8B8A8_UNORM;
        atlasDesc.initialData = atlas.data();
        m_blockAtlas = renderer->CreateTexture(atlasDesc);
        LOG_INFO("Procedural block texture atlas");
    }
        // 8. Crosshair mesh
    {
        MeshData ch;
        ch.vertexStride = 8 * sizeof(f32);
        f32 s = 0.5f;
        f32 len = 4.0f;
        f32 verts[] = {
            -len, -s, 0,  0,0,1,  0,0,
             len, -s, 0,  0,0,1,  1,0,
             len,  s, 0,  0,0,1,  1,1,
            -len,  s, 0,  0,0,1,  0,1,
             -s, -len, 0, 0,0,1,  0,0,
              s, -len, 0, 0,0,1,  1,0,
              s,  len, 0, 0,0,1,  1,1,
             -s,  len, 0, 0,0,1,  0,1,
        };
        ch.vertices.assign(verts, verts + 64);
        ch.vertexCount = 8;
        u32 idx[] = { 0,1,2, 0,2,3, 4,5,6, 4,6,7 };
        ch.indices.assign(idx, idx + 12);
        ch.indexCount = 12;
        m_crosshairMesh = renderer->CreateMesh(ch);
    }
    // 8b. White unit quad mesh for UI rendering
    {
        MeshData qm;
        qm.vertexStride = 8 * sizeof(f32);
        f32 verts[] = {
            0,0,0, 0,0,1, 0,0,
            1,0,0, 0,0,1, 1,0,
            1,1,0, 0,0,1, 1,1,
            0,1,0, 0,0,1, 0,1,
        };
        qm.vertices.assign(verts, verts + 32);
        qm.vertexCount = 4;
        u32 idx[] = { 0,1,2, 0,2,3 };
        qm.indices.assign(idx, idx + 6);
        qm.indexCount = 6;
        m_whiteQuadMesh = renderer->CreateMesh(qm);
    }
    // 8c. Held block mesh (first-person cube)
    {
        MeshData hb;
        hb.vertexStride = 10 * sizeof(f32);
        f32 bt = (f32)(u8)BlockType::Dirt / 255.0f;
        auto addFace = [&](f32 x0,f32 y0,f32 z0, f32 x1,f32 y1,f32 z1, f32 x2,f32 y2,f32 z2, f32 x3,f32 y3,f32 z3,
                           f32 nx,f32 ny,f32 nz, int fi) {
            f32 fv = (f32)fi / 6.0f;
            struct V { f32 x,y,z; };
            V verts[4] = {{x0,y0,z0},{x1,y1,z1},{x2,y2,z2},{x3,y3,z3}};
            for (int i = 0; i < 4; i++) {
                hb.vertices.push_back(verts[i].x); hb.vertices.push_back(verts[i].y); hb.vertices.push_back(verts[i].z);
                hb.vertices.push_back(nx); hb.vertices.push_back(ny); hb.vertices.push_back(nz);
                hb.vertices.push_back(bt); hb.vertices.push_back(fv);
                f32 lx = verts[i].x + 0.5f, ly = verts[i].y + 0.5f, lz = verts[i].z + 0.5f;
                if (fi == 0) { hb.vertices.push_back(1.0f-lz); hb.vertices.push_back(ly); }
                else if (fi == 1) { hb.vertices.push_back(lz); hb.vertices.push_back(ly); }
                else if (fi == 2) { hb.vertices.push_back(lx); hb.vertices.push_back(1.0f-lz); }
                else if (fi == 3) { hb.vertices.push_back(lx); hb.vertices.push_back(lz); }
                else if (fi == 4) { hb.vertices.push_back(1.0f-lx); hb.vertices.push_back(ly); }
                else { hb.vertices.push_back(lx); hb.vertices.push_back(ly); }
            }
        };
        addFace(0.5f,-0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f,-0.5f, 0.5f,-0.5f,-0.5f, 1,0,0, 0);
        addFace(-0.5f,-0.5f,-0.5f, -0.5f, 0.5f,-0.5f, -0.5f, 0.5f, 0.5f, -0.5f,-0.5f, 0.5f, -1,0,0, 1);
        addFace(-0.5f, 0.5f,-0.5f, 0.5f, 0.5f,-0.5f, 0.5f, 0.5f, 0.5f, -0.5f, 0.5f, 0.5f, 0,1,0, 2);
        addFace(-0.5f,-0.5f, 0.5f, 0.5f,-0.5f, 0.5f, 0.5f,-0.5f,-0.5f, -0.5f,-0.5f,-0.5f, 0,-1,0, 3);
        addFace(0.5f,-0.5f, 0.5f, 0.5f, 0.5f, 0.5f, -0.5f, 0.5f, 0.5f, -0.5f,-0.5f, 0.5f, 0,0,1, 4);
        addFace(-0.5f,-0.5f,-0.5f, -0.5f, 0.5f,-0.5f, 0.5f, 0.5f,-0.5f, 0.5f,-0.5f,-0.5f, 0,0,-1, 5);
        hb.vertexCount = 24;
        u32 hbIdx[] = {0,1,2,0,2,3, 4,5,6,4,6,7, 8,9,10,8,10,11, 12,13,14,12,14,15, 16,17,18,16,18,19, 20,21,22,20,22,23};
        hb.indices.assign(hbIdx, hbIdx + 36);
        hb.indexCount = 36;
        m_heldBlockMesh = renderer->CreateMesh(hb);
    }
    // 8e. Skybox mesh
    {
        MeshData sb;
        sb.vertexStride = 8 * sizeof(f32);
        f32 sbVerts[] = {
             1,-1,-1, 1,0,0, 0,0,  1, 1,-1, 1,0,0, 1,0,  1, 1, 1, 1,0,0, 1,1,  1,-1, 1, 1,0,0, 0,1,
            -1,-1, 1,-1,0,0, 0,0, -1, 1, 1,-1,0,0, 1,0, -1, 1,-1,-1,0,0, 1,1, -1,-1,-1,-1,0,0, 0,1,
            -1, 1,-1, 0,1,0, 0,0,  1, 1,-1, 0,1,0, 1,0,  1, 1, 1, 0,1,0, 1,1, -1, 1, 1, 0,1,0, 0,1,
            -1,-1, 1, 0,-1,0, 0,0, 1,-1, 1, 0,-1,0, 1,0, 1,-1,-1, 0,-1,0, 1,1, -1,-1,-1, 0,-1,0, 0,1,
            -1,-1, 1, 0,0,1, 0,0,  1,-1, 1, 0,0,1, 1,0,  1, 1, 1, 0,0,1, 1,1, -1, 1, 1, 0,0,1, 0,1,
             1,-1,-1, 0,0,-1, 0,0, -1,-1,-1, 0,0,-1, 1,0, -1, 1,-1, 0,0,-1, 1,1,  1, 1,-1, 0,0,-1, 0,1,
        };
        sb.vertices.assign(sbVerts, sbVerts + 192);
        sb.vertexCount = 24;
        u32 sbIdx[] = {0,1,2,0,2,3, 4,5,6,4,6,7, 8,9,10,8,10,11, 12,13,14,12,14,15, 16,17,18,16,18,19, 20,21,22,20,22,23};
        sb.indices.assign(sbIdx, sbIdx + 36);
        sb.indexCount = 36;
        m_skyboxMesh = renderer->CreateMesh(sb);
    }

    m_initialized = true;
    LOG_INFO("VoxelGame initialized");
    return true;
}

void VoxelGame::SetupShadersAndPipeline(Renderer* renderer) {
    // Create UI pipeline FIRST (simple shaders that always work)
    {
        const char* uiVS = R"(#version 330 core
            layout(location=0) in vec3 a_Position;
            uniform mat4 u_Projection;
            uniform mat4 u_Model;
            void main() {
                gl_Position = u_Projection * u_Model * vec4(a_Position, 1.0);
            }
        )";
        const char* uiFS = R"(#version 330 core
            uniform vec4 u_Color;
            out vec4 FragColor;
            void main() { FragColor = u_Color; }
        )";
        ShaderDesc uVS; uVS.stage = ShaderStage::Vertex; uVS.source = uiVS;
        ShaderDesc uFS; uFS.stage = ShaderStage::Fragment; uFS.source = uiFS;
        m_uiVS = renderer->CreateShader(uVS);
        m_uiFS = renderer->CreateShader(uFS);
        if (m_uiVS != kInvalidHandle && m_uiFS != kInvalidHandle) {
            PipelineStateDesc up;
            up.vertexShader = &uVS; up.fragmentShader = &uFS;
            up.topology = PrimitiveTopology::TriangleList;
            up.cullMode = CullMode::None;
            up.depthTestEnabled = false; up.depthWriteEnabled = false;
            up.blendMode = BlendMode::AlphaBlend;
            up.vertexLayout = { {"POSITION", Format::R32G32B32_FLOAT, 0, 0},
                                {"NORMAL", Format::R32G32B32_FLOAT, 12, 0},
                                {"TEXCOORD", Format::R32G32_FLOAT, 24, 0} };
            m_uiPipeline = renderer->CreatePipelineState(up);
        }
    }
    
    // Try to create world shaders and pipeline (may fail on some GLSL)
    m_worldVSDesc.stage = ShaderStage::Vertex;
    m_worldVSDesc.source = kWorldVertexSrc;
    m_worldVS = renderer->CreateShader(m_worldVSDesc);

    m_worldFSDesc.stage = ShaderStage::Fragment;
    m_worldFSDesc.source = kWorldFragmentSrc;
    m_worldFS = renderer->CreateShader(m_worldFSDesc);

    if (m_worldVS != kInvalidHandle && m_worldFS != kInvalidHandle) {
        PipelineStateDesc pd;
        pd.vertexShader = &m_worldVSDesc;
        pd.fragmentShader = &m_worldFSDesc;
        pd.topology = PrimitiveTopology::TriangleList;
        pd.cullMode = CullMode::Back;
        pd.fillMode = FillMode::Solid;
        pd.depthTestEnabled = true;
        pd.depthWriteEnabled = true;
        pd.depthFunc = CompareFunction::Less;
        pd.vertexLayout = {
            {"POSITION", Format::R32G32B32_FLOAT, 0, 0},
            {"NORMAL",   Format::R32G32B32_FLOAT, 12, 0},
            {"TEXCOORD", Format::R32G32_FLOAT, 24, 0},
            };
        m_worldPipeline = renderer->CreatePipelineState(pd);
        if (m_worldPipeline == kInvalidHandle) {
            LOG_ERROR("Failed to create world pipeline - will render UI only");
        }
    } else {
        LOG_ERROR("Failed to create world shaders - will render UI only");
    }
    
    // Skybox pipeline
    {
        ShaderDesc svs; svs.stage = ShaderStage::Vertex; svs.source = kSkyboxVertexSrc;
        ShaderDesc sfs; sfs.stage = ShaderStage::Fragment; sfs.source = kSkyboxFragmentSrc;
        m_skyboxVS = renderer->CreateShader(svs);
        m_skyboxFS = renderer->CreateShader(sfs);
        if (m_skyboxVS != kInvalidHandle && m_skyboxFS != kInvalidHandle) {
            PipelineStateDesc sp;
            sp.vertexShader = &svs; sp.fragmentShader = &sfs;
            sp.topology = PrimitiveTopology::TriangleList;
            sp.cullMode = CullMode::None;
            sp.depthTestEnabled = true; sp.depthWriteEnabled = false;
            sp.depthFunc = CompareFunction::LessEqual;
            sp.blendMode = BlendMode::Opaque;
            sp.vertexLayout = { {"POSITION", Format::R32G32B32_FLOAT, 0, 0} };
            m_skyboxPipeline = renderer->CreatePipelineState(sp);
        }
    }
    
    m_pipelineCreated = true;
}

void VoxelGame::Update(Engine* engine, f32 deltaTime) {
    if (!m_initialized) return;
     m_gameTime += deltaTime;
     UpdateChunks(engine);
     HandleBlockInteraction(engine);
     m_player.Update(engine, deltaTime);
     // Update water physics
     {
         Vec3 ppos = m_player.GetPosition();
         m_world->UpdateWaterPhysics(deltaTime, (i32)ppos.x, (i32)ppos.z);
     }
     // Dynamic FOV: expand when sprinting, contract when walking
     {
         Camera* cam = m_player.GetCamera();
         f32 sprintFrac = m_player.GetSprintFrac();
         f32 targetFOV = 70.0f + sprintFrac * 10.0f; // 70° base, up to 80° sprinting
         if (m_player.IsSneaking()) targetFOV = 70.0f;
         cam->SetTargetFOV(targetFOV);
         cam->UpdateFOV(deltaTime);
     }
     m_particleSystem.Update(deltaTime);
     if (m_placeCooldown > 0.0f) m_placeCooldown -= deltaTime;
     HandleHotbarSelection(engine->GetInput());
}

void VoxelGame::HandleBlockInteraction(Engine* engine) {
    auto* input = engine->GetInput();
    auto* camera = m_player.GetCamera();
    Vec3 eyePos = m_player.GetPosition();
    Vec3 forward = camera->GetForward();
    m_raycastResult = BlockRaycast::Cast(eyePos, forward, 8.0f, m_world);

    if (input->IsMouseDown(0) && m_raycastResult.hit) {
        BlockType block = m_world->GetBlock(m_raycastResult.blockX,
                                             m_raycastResult.blockY,
                                             m_raycastResult.blockZ);
        if (block != BlockType::Air && block != BlockType::Water) {
            Vec3 wp((f32)m_raycastResult.blockX + 0.5f,
                    (f32)m_raycastResult.blockY + 0.5f,
                    (f32)m_raycastResult.blockZ + 0.5f);
            Vec3 bc = BlockInfo::GetFaceColor(block, BlockFace::Top);
            m_world->SetBlock(m_raycastResult.blockX, m_raycastResult.blockY,
                              m_raycastResult.blockZ, BlockType::Air);
            m_particleSystem.SpawnBreakParticles(wp, bc, 12);
            m_soundManager.PlayBlockBreak();
        }
    }

    if (input->IsMousePressed(1) && m_raycastResult.hit && m_placeCooldown <= 0.0f) {
        BlockType placeBlock = GetSelectedBlock();
        BlockType existing = m_world->GetBlock(m_raycastResult.placeX,
                                                m_raycastResult.placeY,
                                                m_raycastResult.placeZ);
        if (existing == BlockType::Air && BlockInfo::IsSolid(placeBlock)) {
            m_world->SetBlock(m_raycastResult.placeX, m_raycastResult.placeY,
                              m_raycastResult.placeZ, placeBlock);
            m_soundManager.PlayBlockPlace();
            m_placeCooldown = 0.15f;
        }
    }

    if (input->IsKeyPressed(VK_ESCAPE)) engine->Stop();
}

void VoxelGame::UpdateChunks(Engine* engine) {
    (void)engine;
    Vec3 p = m_player.GetPosition();
    m_world->UpdateChunksAround((i32)p.x, (i32)p.z, 4);
}

void VoxelGame::Render(Engine* engine, f32 deltaTime) {
    if (!m_initialized || !m_pipelineCreated) return;
    auto* renderer = engine->GetRenderer();
    auto* camera = m_player.GetCamera();
    auto* window = engine->GetWindow();
    f32 ww = (f32)window->GetWidth();
    f32 wh = (f32)window->GetHeight();
    
    // Skybox (render first, behind everything)
    if (m_skyboxPipeline != kInvalidHandle && m_skyboxMesh != kInvalidHandle) {
        renderer->BindPipelineState(m_skyboxPipeline);
        Mat4 skyView = glm::mat4(glm::mat3(camera->GetViewMatrix()));
        renderer->SetUniformMat4("u_View", skyView);
        renderer->SetUniformMat4("u_Projection", camera->GetProjectionMatrix());
        renderer->DrawMesh(m_skyboxMesh);
    }
    
    // Render world if world pipeline is available
    if (m_worldPipeline != kInvalidHandle) {
        RenderWorld(renderer, camera);
        m_particleSystem.Render(renderer);
        RenderHeldBlock(renderer, camera);
    }
    
    // Always render UI
    RenderUI(renderer, ww, wh);
    RenderHotbar(renderer, ww, wh);
}

void VoxelGame::RenderWorld(Renderer* renderer, Camera* camera) {
    renderer->BindPipelineState(m_worldPipeline);
    renderer->SetUniformMat4("u_View", camera->GetViewMatrix());
    renderer->SetUniformMat4("u_Projection", camera->GetProjectionMatrix());
    renderer->SetUniformVec3("u_CameraPos", camera->GetPosition());
    Vec3 ld = glm::normalize(Vec3(0.5f, -0.8f, 0.3f));
    renderer->SetUniformVec3("u_LightDir", ld);
    renderer->SetUniformVec3("u_LightColor", Vec3(1.0f, 0.95f, 0.85f) * 1.2f);
    renderer->SetUniformFloat("u_Time", m_gameTime);
    renderer->SetUniformFloat("u_AtlasGridX", 4.0f);
    renderer->SetUniformFloat("u_AtlasGridY", 3.0f);
    if (m_blockAtlas != kInvalidHandle) {
        renderer->BindTexture(m_blockAtlas, 0);
        renderer->SetUniformInt("u_BlockAtlas", 0);
    }
    // Temporarily disable frustum culling for debugging
    m_world->Render(renderer, nullptr);
}

void VoxelGame::RenderUI(Renderer* renderer, f32 winW, f32 winH) {
    if (m_uiPipeline == kInvalidHandle) return;
    renderer->BindPipelineState(m_uiPipeline);
    Mat4 ortho = glm::ortho(0.0f, winW, 0.0f, winH, -1.0f, 1.0f);
    renderer->SetUniformMat4("u_Projection", ortho);
    renderer->SetUniformVec4("u_Color", Vec4(1.0f, 1.0f, 1.0f, 0.8f));

    if (m_crosshairMesh != kInvalidHandle) {
        Mat4 model = Translate(Mat4(1.0f), Vec3(winW * 0.5f, winH * 0.5f, 0.0f));
        renderer->SetUniformMat4("u_Model", model);
        renderer->DrawMesh(m_crosshairMesh);
    }
}

void VoxelGame::RenderCrosshair(Renderer* renderer) { (void)renderer; }
void VoxelGame::RenderDebugInfo(Engine* engine) { (void)engine; }


// ============================================================
// Hotbar: Block Selection
// ============================================================
void VoxelGame::HandleHotbarSelection(Input* input) {
    // Number keys 1-9 select hotbar slot
    for (i32 i = 0; i < kHotbarSlots && i < 9; ++i) {
        if (input->IsKeyPressed('1' + i)) {
            m_selectedSlot = i;
        }
    }
    // Scroll wheel cycles through slots
    i32 scroll = input->GetScrollDelta();
    if (scroll > 0) {
        m_selectedSlot = (m_selectedSlot - 1 + kHotbarSlots) % kHotbarSlots;
    } else if (scroll < 0) {
        m_selectedSlot = (m_selectedSlot + 1) % kHotbarSlots;
    }
}

BlockType VoxelGame::GetSelectedBlock() const {
    // Map hotbar slot to block type
    static const BlockType hotbarBlocks[kHotbarSlots] = {
        BlockType::Dirt,
        BlockType::Stone,
        BlockType::Cobblestone,
        BlockType::OakPlanks,
        BlockType::OakLog,
        BlockType::Sand,
        BlockType::Grass,
        BlockType::Snow,
        BlockType::Bedrock,
    };
    if (m_selectedSlot >= 0 && m_selectedSlot < kHotbarSlots) {
        return hotbarBlocks[m_selectedSlot];
    }
    return BlockType::Dirt;
}

void VoxelGame::RenderHotbar(Renderer* renderer, f32 winW, f32 winH) {
    if (m_uiPipeline == kInvalidHandle || m_whiteQuadMesh == kInvalidHandle) return;
    
    renderer->BindPipelineState(m_uiPipeline);
    Mat4 ortho = glm::ortho(0.0f, winW, 0.0f, winH, -1.0f, 1.0f);
    renderer->SetUniformMat4("u_Projection", ortho);
    
    f32 slotSize = 40.0f;
    f32 gap = 4.0f;
    f32 totalWidth = kHotbarSlots * slotSize + (kHotbarSlots - 1) * gap;
    f32 startX = (winW - totalWidth) * 0.5f;
    f32 y = 20.0f;
    
    static const BlockType hotbarBlocks[kHotbarSlots] = {
        BlockType::Dirt, BlockType::Stone, BlockType::Cobblestone,
        BlockType::OakPlanks, BlockType::OakLog, BlockType::Sand,
        BlockType::Grass, BlockType::Snow, BlockType::Bedrock,
    };
    
    for (i32 i = 0; i < kHotbarSlots; ++i) {
        f32 x = startX + i * (slotSize + gap);
        // Slot background
        Vec4 bgColor = (i == m_selectedSlot) ? Vec4(0.4f, 0.4f, 0.7f, 0.9f) : Vec4(0.15f, 0.15f, 0.15f, 0.7f);
        renderer->SetUniformVec4("u_Color", bgColor);
        Mat4 model = Translate(Mat4(1.0f), Vec3(x, y, 0.0f));
        model = Scale(model, Vec3(slotSize, slotSize, 1.0f));
        renderer->SetUniformMat4("u_Model", model);
        renderer->DrawMesh(m_whiteQuadMesh);
        
        // Draw slot block color using the block's top face color
        BlockType bt = hotbarBlocks[i];
        Vec3 bc = BlockInfo::GetFaceColor(bt, BlockFace::Top);
        // If grass, use green top; if oak_log, use side (bark) color
        if (bt == BlockType::OakLog) bc = BlockInfo::GetFaceColor(bt, BlockFace::Left);
        else if (bt == BlockType::Grass) bc = BlockInfo::GetFaceColor(bt, BlockFace::Top);
        renderer->SetUniformVec4("u_Color", Vec4(bc.x, bc.y, bc.z, 1.0f));
        Mat4 bmodel = Translate(Mat4(1.0f), Vec3(x + 4.0f, y + 4.0f, 0.0f));
        bmodel = Scale(bmodel, Vec3(slotSize - 8.0f, slotSize - 8.0f, 1.0f));
        renderer->SetUniformMat4("u_Model", bmodel);
        renderer->DrawMesh(m_whiteQuadMesh);
        
        // Selection highlight
        if (i == m_selectedSlot) {
            renderer->SetUniformVec4("u_Color", Vec4(1.0f, 1.0f, 1.0f, 0.6f));
            Mat4 hmodel = Translate(Mat4(1.0f), Vec3(x - 1.0f, y - 1.0f, 0.0f));
            hmodel = Scale(hmodel, Vec3(slotSize + 2.0f, 1.5f, 1.0f));
            renderer->SetUniformMat4("u_Model", hmodel);
            renderer->DrawMesh(m_whiteQuadMesh);
            hmodel = Translate(Mat4(1.0f), Vec3(x - 1.0f, y + slotSize - 0.5f, 0.0f));
            hmodel = Scale(hmodel, Vec3(slotSize + 2.0f, 1.5f, 1.0f));
            renderer->SetUniformMat4("u_Model", hmodel);
            renderer->DrawMesh(m_whiteQuadMesh);
            hmodel = Translate(Mat4(1.0f), Vec3(x - 1.0f, y, 0.0f));
            hmodel = Scale(hmodel, Vec3(1.5f, slotSize, 1.0f));
            renderer->SetUniformMat4("u_Model", hmodel);
            renderer->DrawMesh(m_whiteQuadMesh);
            hmodel = Translate(Mat4(1.0f), Vec3(x + slotSize - 0.5f, y, 0.0f));
            hmodel = Scale(hmodel, Vec3(1.5f, slotSize, 1.0f));
            renderer->SetUniformMat4("u_Model", hmodel);
            renderer->DrawMesh(m_whiteQuadMesh);
        }
    }
}
void VoxelGame::RenderHeldBlock(Renderer* renderer, Camera* camera) {
    if (m_heldBlockMesh == kInvalidHandle) return;
    // Use the world pipeline (already bound in RenderWorld, or bind again here)
    renderer->BindPipelineState(m_worldPipeline);
    renderer->SetUniformMat4("u_View", camera->GetViewMatrix());
    renderer->SetUniformMat4("u_Projection", camera->GetProjectionMatrix());
    // Held block position: in front and to the right of the camera
    Vec3 pos = camera->GetPosition();
    Vec3 fwd = camera->GetForward();
    Vec3 right = camera->GetRight();
    Vec3 up(0.0f, 1.0f, 0.0f);
    Vec3 heldPos = pos + fwd * 2.5f + right * 1.2f - up * 0.8f;
    Mat4 model = glm::translate(Mat4(1.0f), heldPos);
    model = glm::rotate(model, glm::radians(30.0f), Vec3(0.0f, 1.0f, 0.0f));
    model = glm::scale(model, Vec3(0.4f));
    renderer->SetUniformMat4("u_Model", model);
    // Bind the block atlas (already bound, but set uniforms again)
    renderer->SetUniformVec3("u_CameraPos", camera->GetPosition());
    Vec3 ld = glm::normalize(Vec3(0.5f, -0.8f, 0.3f));
    renderer->SetUniformVec3("u_LightDir", ld);
    renderer->SetUniformVec3("u_LightColor", Vec3(1.0f, 0.95f, 0.85f) * 1.2f);
    renderer->SetUniformFloat("u_Time", m_gameTime);
    renderer->SetUniformFloat("u_AtlasGridX", 4.0f);
    renderer->SetUniformFloat("u_AtlasGridY", 3.0f);
    if (m_blockAtlas != kInvalidHandle) {
        renderer->BindTexture(m_blockAtlas, 0);
        renderer->SetUniformInt("u_BlockAtlas", 0);
    }
    renderer->DrawMesh(m_heldBlockMesh);
}

void VoxelGame::Shutdown(Engine* engine) {
    if (!m_initialized) return;
    m_initialized = false;
    LOG_INFO("VoxelGame shutting down...");
    if (m_pipelineCreated && engine) {
        auto* r = engine->GetRenderer();
        if (r) {
            if (m_worldPipeline  != kInvalidHandle) r->DestroyPipelineState(m_worldPipeline);
            if (m_worldVS        != kInvalidHandle) r->DestroyShader(m_worldVS);
            if (m_worldFS        != kInvalidHandle) r->DestroyShader(m_worldFS);
            if (m_uiPipeline     != kInvalidHandle) r->DestroyPipelineState(m_uiPipeline);
            if (m_uiVS           != kInvalidHandle) r->DestroyShader(m_uiVS);
            if (m_uiFS           != kInvalidHandle) r->DestroyShader(m_uiFS);
            if (m_crosshairMesh != kInvalidHandle) r->DestroyMesh(m_crosshairMesh);
        if (m_whiteQuadMesh != kInvalidHandle) r->DestroyMesh(m_whiteQuadMesh);
        if (m_heldBlockMesh != kInvalidHandle) r->DestroyMesh(m_heldBlockMesh);
        if (m_skyboxMesh != kInvalidHandle) r->DestroyMesh(m_skyboxMesh);
        if (m_skyboxPipeline != kInvalidHandle) r->DestroyPipelineState(m_skyboxPipeline);
        if (m_skyboxVS != kInvalidHandle) r->DestroyShader(m_skyboxVS);
        if (m_skyboxFS != kInvalidHandle) r->DestroyShader(m_skyboxFS);
        }
    }
    m_particleSystem.Shutdown();
    m_soundManager.Shutdown();
    if (m_blockAtlas != kInvalidHandle && engine) {
        auto* r = engine->GetRenderer();
        if (r) r->DestroyTexture(m_blockAtlas);
        m_blockAtlas = kInvalidHandle;
    }

    delete m_world; m_world = nullptr;
    m_initialized = false;
    LOG_INFO("VoxelGame shutting down...");
}

} // namespace painkiller


