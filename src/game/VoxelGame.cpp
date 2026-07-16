#include "VoxelGame.h"
#include "Logger.h"
#include "Math.h"
#include "BlockRaycast.h"
#include "TextureLoader.h"
#include "ResourceManager.h"
#include <cstdlib>
#include <algorithm>
#include <cstdio>
#include <direct.h>
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

    uniform float u_BlockAtlasCell;
    int getBlockType() { 
        if (u_BlockAtlasCell > 0.0) return int(u_BlockAtlasCell * 255.0 + 0.5);
        return int(v_TexCoord.x * 255.0 + 0.5); 
    }
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
        if (type == 8) return 22;          // OakPlanks
        if (type == 9) return 7;            // Sand
        if (type == 10) return -2;          // Water
        if (type == 11) return 8;           // Snow
        if (type == 12) return 9;          // Bedrock
        if (type == 13) return 10;
        if (type == 14) return 11;
        if (type == 15) return 12;
        if (type == 16) return 13;
        if (type == 17) {
            if (face == 2) return 14;
            else if (face == 3) return 16;
            else return 15;
        }
        if (type == 18) {
            if (face == 2 || face == 3) return 16;
            else if (face == 0 || face == 4) return 18;
            else return 17;
        }
        if (type == 19) {
            if (face == 2 || face == 3) return 16;
            else if (face == 0 || face == 4) return 18;
            else return 17;
        }
        if (type == 20) return 19;
        if (type == 21) return 20;
        if (type == 22) return 21;
        if (type == 23) return -1;  // Sticks
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
        
        if (cell == -2) {
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
        
        // Flat colors - no atlas texture sampling needed
        if (bt == 1 && face != 2 && face != 3) {
            // Grass side: green at top, brown at bottom
            color = mix(vec3(0.45,0.32,0.18), vec3(0.29,0.65,0.14), uv.y);
        } else if (bt == 2 || (bt == 1 && face == 3)) {
            color = vec3(0.45,0.32,0.18);  // Dirt brown
        } else if (bt == 3) color = vec3(0.50,0.50,0.50); // Stone
        else if (bt == 4) color = vec3(0.40,0.40,0.40); // Cobble
        else if (bt == 5) color = vec3(0.42,0.29,0.13); // Wood
        else if (bt == 6) { // OakLog
            if (face == 2 || face == 3) color = vec3(0.55,0.37,0.16);
            else color = vec3(0.42,0.29,0.13);
        }
        else if (bt == 7) { // Leaves
            float ld2=fract(v_FragPos.x*3.0+v_FragPos.y*5.0+v_FragPos.z*7.0);
            if (ld2<0.20) discard;
            color = vec3(0.15,0.50,0.08); alpha = 0.85;
        }
        else if (bt == 8) color = vec3(0.52,0.36,0.18);
        else if (bt == 9) color = vec3(0.76,0.70,0.50);
        else if (bt == 10) { // Water
            float wv2=waterWave(v_FragPos,u_Time);
            vec3 wc2=vec3(0.15,0.35,0.75),fc2=vec3(0.3,0.6,0.9);
            color=mix(wc2,fc2,wv2*2.0+0.5); alpha=0.6;
        }
        else if (bt == 11) color = vec3(0.95,0.95,0.98);
        else if (bt == 12) color = vec3(0.20,0.20,0.20);
        else if (bt == 13) color = vec3(0.25,0.25,0.22);
        else if (bt == 14) color = vec3(0.75,0.60,0.50);
        else if (bt == 15) color = vec3(0.85,0.70,0.15);
        else if (bt == 16) color = vec3(0.20,0.70,0.75);
        else if (bt == 17) color = (face==2)?vec3(0.55,0.45,0.30):vec3(0.45,0.35,0.22);
        else if (bt == 18||bt==19) {
            if(face==2||face==3)color=vec3(0.40,0.40,0.40);
            else if(face==0||face==4)color=vec3(0.30,0.30,0.30);
            else color=vec3(0.45,0.40,0.35);
        }
        else if (bt == 20) color = vec3(0.50,0.35,0.15);
        else if (bt == 21) { color=vec3(0.60,0.80,0.90); alpha=0.35; }
        else if (bt == 22) color = vec3(0.45,0.32,0.18);
        else if (bt == 23) color = vec3(0.52,0.36,0.18);
        
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
    uniform float u_Time;
    out vec4 FragColor;
    void main() {
        vec3 dir = normalize(v_Dir);
        float h = dir.y;
        // Day/night cycle based on time
        float dayPhase = sin(u_Time * 0.0005); // -1 to 1 over ~12.5 min
        float dayFactor = dayPhase * 0.5 + 0.5; // 0=night, 1=day
        
        // Day sky
        vec3 daySky = vec3(0.35, 0.60, 0.90);
        vec3 dayHorizon = vec3(0.60, 0.78, 0.95);
        // Night sky (dark blue with stars)
        vec3 nightSky = vec3(0.05, 0.05, 0.15);
        vec3 nightHorizon = vec3(0.10, 0.08, 0.15);
        // Sunset colors
        vec3 sunsetSky = vec3(0.80, 0.40, 0.20);
        vec3 sunsetHorizon = vec3(0.90, 0.55, 0.30);
        
        // Blend based on day phase
        float sunsetFactor = 1.0 - abs(dayPhase) * 3.0;
        sunsetFactor = clamp(sunsetFactor, 0.0, 1.0);
        
        vec3 sky = mix(nightSky, daySky, dayFactor);
        vec3 horizon = mix(nightHorizon, dayHorizon, dayFactor);
        // Add sunset glow when dayFactor is ~0.5
        sky = mix(sky, sunsetSky, sunsetFactor * 0.6);
        horizon = mix(horizon, sunsetHorizon, sunsetFactor * 0.5);
        
        vec3 ground = vec3(0.30, 0.25, 0.20);
        vec3 color = mix(ground, horizon, smoothstep(-0.1, 0.0, h));
        color = mix(color, sky, smoothstep(0.0, 0.4, h));
        
        // Stars at night (subtle dots)
        float starNoise = fract(sin(dir.x * 100.0 + dir.y * 73.0 + dir.z * 57.0) * 43758.5453);
        float stars = step(0.998 - (1.0 - dayFactor) * 0.003, starNoise) * (1.0 - dayFactor);
        color += vec3(stars * 0.8, stars * 0.8, stars * 1.0);
        
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
    m_networkServer.Start(25565);

    // 6. Load initial chunks (try save file first)
    if (!m_world->LoadWorld("saves/world.sav")) {
        UpdateChunks(engine);
        LOG_INFO("No save found, generating new world");
    } else {
        LOG_INFO("World loaded from save");
    }

                        
// 7. Generate procedural textures
    {
        const int kSize = 16; const int kCols = 6, kRows = 4;
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
        for (int c = 0; c < 24; c++)
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
            else if (c == 10) { float v=0.25f+n*0.15f; if(sn(px*5,py*5,c+111)>0.75f)v=0.10f; r=g=b=std::max(0.08f,std::min(0.45f,v)); }
            else if (c == 11) { r=0.70f+n*0.12f; g=0.60f+n2*0.10f; b=0.50f+n*0.10f;
                if(sn(px*4+1,py*4+1,c+222)>0.80f){r+=0.05f;g+=0.03f;b-=0.05f;}}
            else if (c == 12) { r=0.80f+n*0.15f; g=0.70f+n2*0.15f; b=0.20f+n*0.10f;
                if(sn(px*5+2,py*5+2,c+333)>0.80f){r+=0.15f;g+=0.15f;}}
            else if (c == 13) { r=0.30f+n*0.15f; g=0.75f+n2*0.12f; b=0.80f+n*0.10f;
                if(sn(px*5+3,py*5+3,c+444)>0.80f){g+=0.15f;b+=0.10f;}}
            else if (c == 14) { int gx=px/4,gy=py/4; float grid=(gx+gy)%2==0?1.0f:0.92f;
                r=0.50f*n*grid+0.35f; g=0.40f*n2*grid+0.28f; b=0.20f*n*grid+0.15f; }
            else if (c == 15) { float stripe=(float)(px%8)/8.0f; float base=0.40f+n*0.12f;
                if(stripe<0.20f||stripe>0.80f)base-=0.05f;
                r=0.45f+n2*0.10f; g=0.32f+n*0.08f; b=0.18f+n2*0.05f; }
            else if (c == 16) { float v=0.38f+n*0.12f; if(sn(px*3+1,py*3+1,c+55)>0.8f)v-=0.08f;
                v=std::max(0.25f,std::min(0.55f,v)); r=g=b=v; }
            else if (c == 17) { int gx=px/4,gy=py/4; float v=0.42f+n*0.10f;
                if((gx+gy)%2==0)v+=0.05f; else v-=0.05f;
                if(sn(px*2,py*2,c+66)>0.75f)v-=0.08f; r=g=b=std::max(0.25f,std::min(0.55f,v)); }
            else if (c == 18) { float v=0.35f+n*0.10f; int gx=px/4,gy=py/4;
                if(gx>=1&&gx<=2&&gy>=1&&gy<=2){r=0.15f+n*0.10f;g=0.08f+n*0.05f;b=0.05f;}
                else{r=g=b=v;}}
            else if (c == 19) { float v=0.45f+n*0.12f; int band=(px%16<2||py%16<2)?1:0;
                if(band)v=0.25f+n*0.08f; r=0.50f+n2*0.10f+band*0.1f;
                g=0.35f+n*0.08f+band*0.05f; b=0.15f+n2*0.05f+band*0.05f; }
            else if (c == 20) { r=0.55f+n*0.10f; g=0.75f+n2*0.10f; b=0.85f+n*0.10f; }
            else if (c == 21) { int book=(px>=3&&px<=12&&py>=3&&py<=12)?1:0;
                if(book){r=0.10f+n*0.15f;g=0.15f+n2*0.10f;b=0.25f+n*0.15f;
                    if(sn(px*5,py*5,c+77)>0.8f){r+=0.10f;g+=0.05f;b+=0.10f;}}
                else{r=0.45f+n*0.10f;g=0.32f+n2*0.08f;b=0.18f+n*0.05f;}}
            else if (c == 22) { float stripe=(float)(px%8)/8.0f; float base=0.48f+n*0.12f;
                if(stripe<0.15f||stripe>0.85f)base-=0.08f;
                if(sn(px/2,py/2,c+88)>0.6f)base+=0.05f;
                r=(0.55f+n2*0.10f)*base/0.55f; g=(0.38f+n*0.08f)*base/0.55f; b=(0.20f+n2*0.05f)*base/0.55f; }
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
        f32 bt = (f32)(u8)GetSelectedBlock() / 255.0f;
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
    // 9. Wireframe cube mesh for block highlight (12 lines, LINE_LIST)
    {
        MeshData wf;
        wf.vertexStride = 3*sizeof(f32);
        f32 wfVerts[] = {
            -0.5f,-0.5f,-0.5f, 0.5f,-0.5f,-0.5f, 0.5f, 0.5f,-0.5f, -0.5f, 0.5f,-0.5f,
            -0.5f,-0.5f, 0.5f, 0.5f,-0.5f, 0.5f, 0.5f, 0.5f, 0.5f, -0.5f, 0.5f, 0.5f
        };
        wf.vertices.assign(wfVerts, wfVerts+24);
        wf.vertexCount = 8;
        u32 wfIdx[] = {0,1, 1,2, 2,3, 3,0, 4,5, 5,6, 6,7, 7,4, 0,4, 1,5, 2,6, 3,7};
        wf.indices.assign(wfIdx, wfIdx+24);
        wf.indexCount = 24;
        m_wireframeCubeMesh = renderer->CreateMesh(wf);
    }
    // 9b. Drop item mesh (small cube, position-only stride=12, use world pipeline)
    {
        MeshData di;
        di.vertexStride = 8*sizeof(f32);
        f32 v[] = { -0.2f,0,0, 0,1,0, 0,0,  0.2f,0,0, 0,1,0, 1,0,  0.2f,0.2f,0, 0,1,0, 1,1,  -0.2f,0.2f,0, 0,1,0, 0,1,
                    -0.2f,0,0.2f, 0,1,0, 0,0,  0.2f,0,0.2f, 0,1,0, 1,0,  0.2f,0.2f,0.2f, 0,1,0, 1,1,  -0.2f,0.2f,0.2f, 0,1,0, 0,1 };
        di.vertices.assign(v, v+64);
        di.vertexCount = 8;
        u32 idx[] = {0,1,2,0,2,3, 4,5,6,4,6,7, 0,4,7,0,7,3, 1,5,6,1,6,2, 3,7,6,3,6,2, 0,1,5,0,5,4};
        di.indices.assign(idx, idx+36);
        di.indexCount = 36;
        m_dropItemMesh = renderer->CreateMesh(di);
    }
    // 10. Player cube mesh (unit cube, position-only stride=12)
    {
        MeshData pc;
        pc.vertexStride = 3*sizeof(f32);
        f32 v[] = {
            -0.5f,-0.5f,-0.5f, 0.5f,-0.5f,-0.5f, 0.5f, 0.5f,-0.5f, -0.5f, 0.5f,-0.5f,
            -0.5f,-0.5f, 0.5f, 0.5f,-0.5f, 0.5f, 0.5f, 0.5f, 0.5f, -0.5f, 0.5f, 0.5f
        };
        pc.vertices.assign(v, v+24);
        pc.vertexCount = 8;
        u32 idx[] = {0,1,2,0,2,3, 4,5,6,4,6,7, 0,4,7,0,7,3, 1,5,6,1,6,2, 3,7,6,3,6,2, 0,1,5,0,5,4};
        pc.indices.assign(idx, idx+36);
        pc.indexCount = 36;
        m_playerCubeMesh = renderer->CreateMesh(pc);
    }

    // Initialize hotbar blocks
    m_hotbarBlocks[0] = BlockType::Dirt;
    m_hotbarBlocks[1] = BlockType::Stone;
    m_hotbarBlocks[2] = BlockType::Cobblestone;
    m_hotbarBlocks[3] = BlockType::OakPlanks;
    m_hotbarBlocks[4] = BlockType::OakLog;
    m_hotbarBlocks[5] = BlockType::Sand;
    m_hotbarBlocks[6] = BlockType::Grass;
    m_hotbarBlocks[7] = BlockType::CraftingTable;
    m_hotbarBlocks[8] = BlockType::Furnace;
    
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
        pd.cullMode = CullMode::None;
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
    // Wireframe pipeline for block highlight (LINE_LIST)
    {
        const char* wfVS = R"(#version 330 core
            layout(location=0) in vec3 a_Position;
            uniform mat4 u_View; uniform mat4 u_Projection;
            uniform mat4 u_Model;
            void main() { gl_Position = u_Projection * u_View * u_Model * vec4(a_Position, 1.0); }
        )";
        const char* wfFS = R"(#version 330 core
            uniform vec4 u_Color; out vec4 FragColor;
            void main() { FragColor = u_Color; }
        )";
        ShaderDesc wvs; wvs.stage=ShaderStage::Vertex; wvs.source=wfVS;
        ShaderDesc wfs; wfs.stage=ShaderStage::Fragment; wfs.source=wfFS;
        m_wireframeVS=renderer->CreateShader(wvs);
        m_wireframeFS=renderer->CreateShader(wfs);
        if(m_wireframeVS!=kInvalidHandle && m_wireframeFS!=kInvalidHandle){
            PipelineStateDesc wp;
            wp.vertexShader=&wvs; wp.fragmentShader=&wfs;
            wp.topology=PrimitiveTopology::LineList;
            wp.cullMode=CullMode::None;
            wp.depthTestEnabled=true; wp.depthWriteEnabled=false;
            wp.depthFunc=CompareFunction::Less;
            wp.vertexLayout={{"POSITION",Format::R32G32B32_FLOAT,0,0}};
            m_wireframePipeline=renderer->CreatePipelineState(wp);
        }
    }
    // Mob pipeline (simple colored 3D, same as player)
    {
        const char* pVS = R"(#version 330 core
            layout(location=0) in vec3 a_Position;
            uniform mat4 u_View; uniform mat4 u_Projection; uniform mat4 u_Model;
            void main() { gl_Position = u_Projection * u_View * u_Model * vec4(a_Position, 1.0); }
        )";
        const char* pFS = R"(#version 330 core
            uniform vec4 u_Color; out vec4 FragColor;
            void main() { FragColor = u_Color; }
        )";
        ShaderDesc pvs; pvs.stage=ShaderStage::Vertex; pvs.source=pVS;
        ShaderDesc pfs; pfs.stage=ShaderStage::Fragment; pfs.source=pFS;
        m_mobVS=renderer->CreateShader(pvs);
        m_mobFS=renderer->CreateShader(pfs);
        if(m_mobVS!=kInvalidHandle && m_mobFS!=kInvalidHandle){
            PipelineStateDesc pp;
            pp.vertexShader=&pvs; pp.fragmentShader=&pfs;
            pp.topology=PrimitiveTopology::TriangleList;
            pp.cullMode=CullMode::Back;
            pp.depthTestEnabled=true; pp.depthWriteEnabled=true;
            pp.blendMode=BlendMode::Opaque;
            pp.vertexLayout={{"POSITION",Format::R32G32B32_FLOAT,0,0}};
            m_mobPipeline=renderer->CreatePipelineState(pp);
        }
    }
    // Player model pipeline (simple colored 3D)
    {
        const char* pVS = R"(#version 330 core
            layout(location=0) in vec3 a_Position;
            uniform mat4 u_View; uniform mat4 u_Projection; uniform mat4 u_Model;
            void main() { gl_Position = u_Projection * u_View * u_Model * vec4(a_Position, 1.0); }
        )";
        const char* pFS = R"(#version 330 core
            uniform vec4 u_Color; out vec4 FragColor;
            void main() { FragColor = u_Color; }
        )";
        ShaderDesc pvs; pvs.stage=ShaderStage::Vertex; pvs.source=pVS;
        ShaderDesc pfs; pfs.stage=ShaderStage::Fragment; pfs.source=pFS;
        m_playerVS=renderer->CreateShader(pvs);
        m_playerFS=renderer->CreateShader(pfs);
        if(m_playerVS!=kInvalidHandle && m_playerFS!=kInvalidHandle){
            PipelineStateDesc pp;
            pp.vertexShader=&pvs; pp.fragmentShader=&pfs;
            pp.topology=PrimitiveTopology::TriangleList;
            pp.cullMode=CullMode::Back;
            pp.depthTestEnabled=true; pp.depthWriteEnabled=true;
            pp.blendMode=BlendMode::Opaque;
            pp.vertexLayout={{"POSITION",Format::R32G32B32_FLOAT,0,0}};
            m_playerPipeline=renderer->CreatePipelineState(pp);
        }
    }
    
    m_pipelineCreated = true;
}

void VoxelGame::Update(Engine* engine, f32 deltaTime) {
    if (!m_initialized) return;
     m_gameTime += deltaTime;
     UpdateChunks(engine);
     m_world->ProcessChunkLoadQueue();
     HandleBlockInteraction(engine, deltaTime);
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
     // Toggle debug mode with F3
     if (engine->GetInput()->IsKeyPressed(VK_F3)) {
         m_isDebugMode = !m_isDebugMode;
     }
     m_particleSystem.Update(deltaTime);
     // Network: broadcast player position to connected clients
     if (m_networkServer.GetClientCount() > 0) {
         Packet p;
         p.WriteU8(1); // PacketType_PlayerPos
         Vec3 pp = m_player.GetPosition();
         p.WriteF32(pp.x); p.WriteF32(pp.y); p.WriteF32(pp.z);
         p.WriteF32(m_player.GetCamera()->GetYaw());
         p.WriteF32(m_player.GetCamera()->GetPitch());
         m_networkServer.Broadcast(p.GetData().data(), (u32)p.GetData().size());
     }
     m_networkServer.Update();
     if (m_placeCooldown > 0.0f) m_placeCooldown -= deltaTime;
     HandleHotbarSelection(engine->GetInput());
     UpdateMobs(deltaTime);
}

void VoxelGame::HandleBlockInteraction(Engine* engine, f32 deltaTime) {
    auto* input = engine->GetInput();
    auto* camera = m_player.GetCamera();
    Vec3 eyePos = m_player.GetPosition();
    Vec3 forward = camera->GetForward();
    m_raycastResult = BlockRaycast::Cast(eyePos, forward, 8.0f, m_world);

    // Block breaking with timer delay
    if (input->IsMouseDown(0) && m_raycastResult.hit) {
        BlockType block = m_world->GetBlock(m_raycastResult.blockX, m_raycastResult.blockY, m_raycastResult.blockZ);
        if (block != BlockType::Air && block != BlockType::Water) {
            if (!m_isBreaking || m_breakX != m_raycastResult.blockX ||
                m_breakY != m_raycastResult.blockY || m_breakZ != m_raycastResult.blockZ) {
                m_isBreaking = true; m_breakTimer = 0.0f;
                m_breakX = m_raycastResult.blockX; m_breakY = m_raycastResult.blockY; m_breakZ = m_raycastResult.blockZ;
            }
            f32 h = BlockInfo::GetHardness(block); if (h <= 0.0f) h = 0.3f;
            // Apply tool multiplier based on held block type
            h *= GetBreakMultiplier();
            m_breakTimer += deltaTime;
            if (m_breakTimer >= h) {
                Vec3 wp((f32)m_breakX+0.5f,(f32)m_breakY+0.5f,(f32)m_breakZ+0.5f);
                Vec3 bc = BlockInfo::GetFaceColor(block, BlockFace::Top);
                m_world->SetBlock(m_breakX,m_breakY,m_breakZ,BlockType::Air);
                m_particleSystem.SpawnBreakParticles(wp,bc,12);
                m_soundManager.PlayBlockBreak();
                // Spawn block drop item
                SpawnBlockDrop(wp, block);
                m_isBreaking = false; m_breakTimer = 0.0f;
            }
        } else { m_isBreaking = false; m_breakTimer = 0.0f; }
    } else { m_isBreaking = false; m_breakTimer = 0.0f; }

    // Inventory click-to-select (when inventory is open, left click to select block for hotbar)
    if (m_inventoryOpen && input->IsMousePressed(0)) {
        f32 mx = (f32)input->GetMouseX();
        f32 winW = (f32)engine->GetWindow()->GetWidth();
        f32 winH = (f32)engine->GetWindow()->GetHeight();
        // Flip Y: Windows mouse Y=0 at top, our ortho Y=0 at bottom
        f32 my = winH - (f32)input->GetMouseY();
        f32 slotSize = 32.0f, gap = 3.0f;
        f32 startX = winW * 0.5f - 4.5f * (slotSize + gap);
        f32 startY = winH * 0.8f;
        
        int blockIdx = 0;
        for (int t = 1; t < (int)BlockType::NumTypes; t++) {
            BlockType bt = (BlockType)t;
            if (bt == BlockType::Air || bt == BlockType::Water || bt == BlockType::Sticks) continue;
            int col = blockIdx % 9;
            int row = blockIdx / 9;
            if (row > 4) break;
            blockIdx++;
            f32 sx = startX + col * (slotSize + gap);
            f32 sy = startY - row * (slotSize + gap);
            if (mx >= sx && mx <= sx + slotSize && my >= sy && my <= sy + slotSize) {
                // Select this block for a hotbar slot
                // Replace current slot's block
                m_selectedSlot = m_selectedSlot; // Keep slot
                m_hotbarBlocks[m_selectedSlot] = bt;
                m_placeCooldown = 0.1f;
            }
        }
    }
    
    // Block placement (right-click)
    if (input->IsMousePressed(1) && m_raycastResult.hit && m_placeCooldown <= 0.0f) {
        // Check for special interactions first
        BlockType hitBlock = m_world->GetBlock(m_raycastResult.blockX,m_raycastResult.blockY,m_raycastResult.blockZ);
        bool handled = false;
        
        if (hitBlock == BlockType::CraftingTable) {
            // Open crafting GUI instead of cycling recipes
            m_craftingOpen = !m_craftingOpen;
            m_furnaceOpen = false;
            m_inventoryOpen = false;
            handled = true;
        } else if (hitBlock == BlockType::Furnace) {
            // Simple furnace: smelt nearby blocks
            i32 cx = m_raycastResult.blockX, cy = m_raycastResult.blockY, cz = m_raycastResult.blockZ;
            // Check if there's an ore nearby and smelt it
            for (int dy = -1; dy <= 1 && !handled; dy++) {
                for (int dz = -1; dz <= 1 && !handled; dz++) {
                    for (int dx = -1; dx <= 1 && !handled; dx++) {
                        BlockType nb = m_world->GetBlock(cx+dx, cy+dy, cz+dz);
                        if (nb == BlockType::IronOre) {
                            m_world->SetBlock(cx+dx, cy+dy, cz+dz, BlockType::Stone);
                            m_world->SetBlock(cx, cy+1, cz, BlockType::IronOre);  // "smelted" result above
                            handled = true;
                        } else if (nb == BlockType::GoldOre) {
                            m_world->SetBlock(cx+dx, cy+dy, cz+dz, BlockType::Stone);
                            m_world->SetBlock(cx, cy+1, cz, BlockType::GoldOre);
                            handled = true;
                        }
                    }
                }
            }
            if (handled) m_placeCooldown = 0.5f;
        } else if (hitBlock == BlockType::Bedrock) {
            // Use bedrock as "bed" - skip to day
            m_gameTime += 24000.0f - fmod(m_gameTime, 24000.0f);
            m_placeCooldown = 0.5f;
            handled = true;
        }
        
        if (!handled) {
            BlockType pb = GetSelectedBlock();
            BlockType ex = m_world->GetBlock(m_raycastResult.placeX,m_raycastResult.placeY,m_raycastResult.placeZ);
            if (ex == BlockType::Air && BlockInfo::IsSolid(pb)) {
                m_world->SetBlock(m_raycastResult.placeX,m_raycastResult.placeY,m_raycastResult.placeZ,pb);
                m_soundManager.PlayBlockPlace();
                m_placeCooldown = 0.15f;
            }
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
        renderer->SetUniformFloat("u_Time", m_gameTime);
        renderer->DrawMesh(m_skyboxMesh);
    }
    
    // Render world if world pipeline is available
    if (m_worldPipeline != kInvalidHandle) {
        RenderWorld(renderer, camera);
        m_particleSystem.Render(renderer);
        RenderHeldBlock(renderer, camera);
    }
    // Block highlight wireframe
    RenderBlockHighlight(renderer, camera);
    RenderPlayerModel(renderer, camera, deltaTime);
    
    // Mobs
    if (m_worldPipeline != kInvalidHandle) {
        RenderDrops(renderer, camera);
        RenderMobs(renderer, camera, deltaTime);
    }
    // Always render UI
    RenderUI(renderer, ww, wh);
    RenderHotbar(renderer, ww, wh);
    if (m_isDebugMode) {
        RenderDebugScreen(renderer, ww, wh);
    }
    if (m_inventoryOpen) {
        RenderInventoryScreen(renderer, ww, wh);
    }
    if (m_craftingOpen) {
        RenderCraftingScreen(renderer, ww, wh);
    }
    if (m_furnaceOpen) {
        RenderFurnaceScreen(renderer, ww, wh);
    }
}

void VoxelGame::RenderWorld(Renderer* renderer, Camera* camera) {
    renderer->BindPipelineState(m_worldPipeline);
    renderer->SetUniformMat4("u_View", camera->GetViewMatrix());
    renderer->SetUniformMat4("u_Projection", camera->GetProjectionMatrix());
    renderer->SetUniformVec3("u_CameraPos", camera->GetPosition());
    // Dynamic sun position based on game time
    float sunAngle = m_gameTime * 0.0005f;
    Vec3 ld = glm::normalize(Vec3(cosf(sunAngle) * 0.8f, -sinf(sunAngle) * 0.8f - 0.2f, sinf(sunAngle * 0.7f) * 0.5f));
    if (ld.y > 0.0f) ld.y = -0.1f; // Never go above horizon
    renderer->SetUniformVec3("u_LightDir", ld);
    // Night time dimmer light
    float dayFactor = sinf(sunAngle) * 0.5f + 0.5f;
    float lightBright = 0.3f + dayFactor * 0.9f;
    renderer->SetUniformVec3("u_LightColor", Vec3(1.0f, 0.95f, 0.85f) * lightBright);
    renderer->SetUniformVec3("u_LightColor", Vec3(1.0f, 0.95f, 0.85f) * 1.2f);
    renderer->SetUniformFloat("u_Time", m_gameTime);
    renderer->SetUniformFloat("u_AtlasGridX", 6.0f);
    renderer->SetUniformFloat("u_AtlasGridY", 4.0f);
    if (m_blockAtlas != kInvalidHandle) {
        renderer->BindTexture(m_blockAtlas, 0);
        renderer->SetUniformInt("u_BlockAtlas", 0);
    }
    // Render world with frustum culling enabled
    Mat4 vp = camera->GetProjectionMatrix() * camera->GetViewMatrix();
    Frustum frustum = Frustum::FromMatrix(vp);
    m_world->Render(renderer, &frustum);
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
    
    // Health bar
    {
        f32 hx = winW * 0.5f - 80.0f, hy = winH - 50.0f;
        f32 hw = 160.0f, hh = 5.0f;
        // Background (dark red)
        renderer->SetUniformVec4("u_Color", Vec4(0.3f, 0.05f, 0.05f, 0.8f));
        Mat4 m = Translate(Mat4(1.0f), Vec3(hx, hy, 0.0f));
        m = Scale(m, Vec3(hw, hh, 1.0f));
        renderer->SetUniformMat4("u_Model", m);
        renderer->DrawMesh(m_whiteQuadMesh);
        // Health (bright red)
        f32 hpFrac = m_health / m_maxHealth;
        renderer->SetUniformVec4("u_Color", Vec4(0.9f, 0.1f, 0.1f, 0.95f));
        m = Translate(Mat4(1.0f), Vec3(hx + 1, hy + 1, 0.0f));
        m = Scale(m, Vec3((hw - 2) * hpFrac, hh - 2, 1.0f));
        renderer->SetUniformMat4("u_Model", m);
        renderer->DrawMesh(m_whiteQuadMesh);
        // Hunger bar background (dark brown)
        renderer->SetUniformVec4("u_Color", Vec4(0.15f, 0.08f, 0.02f, 0.8f));
        m = Translate(Mat4(1.0f), Vec3(hx, hy - 8.0f, 0.0f));
        m = Scale(m, Vec3(hw, hh, 1.0f));
        renderer->SetUniformMat4("u_Model", m);
        renderer->DrawMesh(m_whiteQuadMesh);
        // Hunger (brownish)
        f32 hungerFrac = m_hunger / 20.0f;
        renderer->SetUniformVec4("u_Color", Vec4(0.7f, 0.5f, 0.2f, 0.95f));
        m = Translate(Mat4(1.0f), Vec3(hx + 1, hy - 8.0f + 1, 0.0f));
        m = Scale(m, Vec3((hw - 2) * hungerFrac, hh - 2, 1.0f));
        renderer->SetUniformMat4("u_Model", m);
        renderer->DrawMesh(m_whiteQuadMesh);
    }
}

void VoxelGame::RenderCrosshair(Renderer* renderer) { (void)renderer; }
void VoxelGame::RenderDebugScreen(Renderer* renderer, f32 winW, f32 winH) {
    if (m_uiPipeline == kInvalidHandle || m_whiteQuadMesh == kInvalidHandle) return;
    renderer->BindPipelineState(m_uiPipeline);
    Mat4 ortho = glm::ortho(0.0f, winW, 0.0f, winH, -1.0f, 1.0f);
    renderer->SetUniformMat4("u_Projection", ortho);
    
    // Draw debug bars showing player info (professional F3-style)
    f32 x = 10.0f, y = winH - 20.0f;
    f32 barW = 220.0f, barH = 2.5f, gap = 16.0f;
    f32 smallW = 160.0f;
    
    // Lambda to draw a colored bar (acts like a line of text)
    auto drawBar = [&](const char* label, float r, float g, float b) {
        (void)label;
        renderer->SetUniformVec4("u_Color", Vec4(r, g, b, 0.85f));
        Mat4 m = Translate(Mat4(1.0f), Vec3(x, y, 0.0f));
        m = Scale(m, Vec3(barW, barH, 1.0f));
        renderer->SetUniformMat4("u_Model", m);
        renderer->DrawMesh(m_whiteQuadMesh);
        y -= gap;
    };
    auto drawVar = [&](float val, float r, float g, float b) {
        renderer->SetUniformVec4("u_Color", Vec4(r, g, b, 0.8f));
        Mat4 m = Translate(Mat4(1.0f), Vec3(x+40, y, 0.0f));
        m = Scale(m, Vec3(smallW * std::min(1.0f, val/100.0f), barH, 1.0f));
        renderer->SetUniformMat4("u_Model", m);
        renderer->DrawMesh(m_whiteQuadMesh);
        y -= gap;
    };
    auto drawInfo = [&](const char* label) {
        (void)label;
        renderer->SetUniformVec4("u_Color", Vec4(0.7f, 0.7f, 0.7f, 0.85f));
        Mat4 m = Translate(Mat4(1.0f), Vec3(x, y, 0.0f));
        m = Scale(m, Vec3(barW, barH, 1.0f));
        renderer->SetUniformMat4("u_Model", m);
        renderer->DrawMesh(m_whiteQuadMesh);
        y -= gap;
    };
    
    Vec3 ppos = m_player.GetPosition();
    
    // Section: Position
    drawBar("XYZ", 0.2f, 0.9f, 0.2f);
    drawVar(ppos.x/5, 0.3f, 0.8f, 0.3f);
    drawVar(ppos.y/5, 0.3f, 0.8f, 0.3f);
    drawVar(ppos.z/5, 0.3f, 0.8f, 0.3f);
    y -= gap*0.5f;
    
    // Chunk position
    i32 cx = ((i32)ppos.x) / 16;
    i32 cz = ((i32)ppos.z) / 16;
    drawInfo("CHUNK");
    // Show chunk position with small colored indicator
    renderer->SetUniformVec4("u_Color", Vec4(0.3f, 0.5f, 0.7f, 0.8f));
    Mat4 m = Translate(Mat4(1.0f), Vec3(x+40, y, 0.0f));
    m = Scale(m, Vec3(smallW, barH, 1.0f));
    renderer->SetUniformMat4("u_Model", m);
    renderer->DrawMesh(m_whiteQuadMesh);
    y -= gap;
    
    // Facing direction
    Vec3 fwd = m_player.GetCamera()->GetForward();
    const char* dirStr = "";
    float ax = fabs(fwd.x), az = fabs(fwd.z);
    if (ax > az) dirStr = (fwd.x > 0) ? "+X" : "-X";
    else dirStr = (fwd.z > 0) ? "+Z" : "-Z";
    (void)dirStr;
    drawInfo("FACING");
    renderer->SetUniformVec4("u_Color", Vec4(0.7f, 0.3f, 0.7f, 0.8f));
    m = Translate(Mat4(1.0f), Vec3(x+40, y, 0.0f));
    m = Scale(m, Vec3(smallW, barH, 1.0f));
    renderer->SetUniformMat4("u_Model", m);
    renderer->DrawMesh(m_whiteQuadMesh);
    y -= gap;
    
    // Seed
    drawInfo("SEED");
    renderer->SetUniformVec4("u_Color", Vec4(0.8f, 0.8f, 0.2f, 0.8f));
    m = Translate(Mat4(1.0f), Vec3(x+40, y, 0.0f));
    m = Scale(m, Vec3(smallW, barH, 1.0f));
    renderer->SetUniformMat4("u_Model", m);
    renderer->DrawMesh(m_whiteQuadMesh);
    y -= gap;
    y -= gap*0.5f;
    
    // Targeted block
    drawInfo("TARGET");
    if (m_raycastResult.hit) {
        BlockType bt = m_world->GetBlock(m_raycastResult.blockX, m_raycastResult.blockY, m_raycastResult.blockZ);
        const char* bname = BlockInfo::Get(bt).name;
        (void)bname;
        renderer->SetUniformVec4("u_Color", Vec4(0.2f, 0.5f, 0.9f, 0.8f));
        m = Translate(Mat4(1.0f), Vec3(x+40, y, 0.0f));
        m = Scale(m, Vec3(smallW, barH, 1.0f));
        renderer->SetUniformMat4("u_Model", m);
        renderer->DrawMesh(m_whiteQuadMesh);
    }
    y -= gap;
    
    // Motion indicator
    if (m_player.IsSprinting()) drawInfo("SPRINT");
    else if (m_player.IsSneaking()) drawInfo("SNEAK");
    else if (m_player.IsFlying()) drawInfo("FLY");
    if (m_player.IsOnGround()) drawInfo("GND");
    else drawInfo("AIR ");
    
    // Biomes
    i32 bx = (i32)ppos.x, bz = (i32)ppos.z;
    (void)bx; (void)bz;
    drawInfo("BIOME");
}


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
    if (m_selectedSlot >= 0 && m_selectedSlot < kHotbarSlots) {
        return m_hotbarBlocks[m_selectedSlot];
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
        BlockType bt = m_hotbarBlocks[i];
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
    renderer->SetUniformFloat("u_AtlasGridX", 6.0f);
    renderer->SetUniformFloat("u_AtlasGridY", 4.0f);
    if (m_blockAtlas != kInvalidHandle) {
        renderer->BindTexture(m_blockAtlas, 0);
        renderer->SetUniformInt("u_BlockAtlas", 0);
    }
    renderer->DrawMesh(m_heldBlockMesh);
}

struct BodyPart { float x,y,z, sx,sy,sz; float r,g,b; };
void VoxelGame::RenderPlayerModel(Renderer* renderer, Camera* camera, f32 deltaTime) {
    if(m_playerPipeline==kInvalidHandle||m_playerCubeMesh==kInvalidHandle)return;
    static float walkTime=0; walkTime+=deltaTime;
    bool moving=m_player.IsMoving();
    bool sprint=m_player.IsSprinting();
    
    // Walking animation speed based on movement
    float animSpeed=moving?(sprint?14.0f:10.0f):0.0f;
    float limbSin=0;
    if(moving) limbSin=sinf(walkTime*animSpeed);
    
    // Body part positions/animation: head, body, leftArm, rightArm, leftLeg, rightLeg
    // Using Minecraft-like proportions (total height ~1.8 blocks)
    float bodyWidth=0.6f, bodyDepth=0.3f;
    float headSize=0.5f;
    float armWidth=0.15f, armLen=0.7f;
    float legWidth=0.15f, legLen=0.7f;
    float torsoHeight=0.65f;
    
    // Limb swing: arms swing opposite to legs
    float armSwing=limbSin*0.5f;  // max ~0.5 swing
    float legSwing=limbSin*0.5f;
    
    // Sprint lean
    float lean=sprint?0.15f:0.0f;
    
    // Colors (Minecraft Steve-style)
    float hR=0.8f,hG=0.6f,hB=0.4f;  // head: skin
    float bR=0.2f,bG=0.5f,bB=0.8f;  // body: blue shirt
    float aR=0.8f,aG=0.6f,aB=0.4f;  // arms: skin
    float lR=0.2f,lG=0.3f,lB=0.6f;  // legs: blue jeans
    
    Vec3 pos=camera->GetPosition();
    Vec3 fwd=camera->GetForward();
    Vec3 right=camera->GetRight();
    Vec3 up(0,1,0);
    
    // Player model position: 3 blocks behind camera
    Vec3 playerPos=pos-fwd*3.0f+Vec3(0,-0.9f,0);
    
    renderer->BindPipelineState(m_playerPipeline);
    renderer->SetUniformMat4("u_View", camera->GetViewMatrix());
    renderer->SetUniformMat4("u_Projection", camera->GetProjectionMatrix());
    
    // Helper lambda to draw a body part
    auto drawPart = [&](Vec3 offset, Vec3 scale, Vec3 color, float rotX=0, float rotZ=0) {
        Mat4 m=glm::translate(Mat4(1.0f), playerPos+offset);
        // Apply rotations for animation
        if(rotX!=0)m=glm::rotate(m,rotX,Vec3(1,0,0));
        if(rotZ!=0)m=glm::rotate(m,rotZ,Vec3(0,0,1));
        m=glm::scale(m, scale);
        renderer->SetUniformMat4("u_Model", m);
        renderer->SetUniformVec4("u_Color", Vec4(color.x,color.y,color.z,1.0f));
        renderer->DrawMesh(m_playerCubeMesh);
    };
    
    Vec3 skin(hR,hG,hB), shirt(bR,bG,bB), arm(aR,aG,aB), pants(lR,lG,lB);
    Vec3 darkSkin(hR*0.7f,hG*0.7f,hB*0.7f);
    
    // Head
    drawPart(Vec3(0,1.4f-lean,0), Vec3(headSize,headSize,headSize), skin);
    // Head top (slightly darker)
    drawPart(Vec3(0,1.4f-lean+0.25f,0), Vec3(headSize*0.9f,headSize*0.1f,headSize*0.9f), darkSkin);
    
    // Body/torso
    drawPart(Vec3(0,0.85f-lean,0), Vec3(bodyWidth,torsoHeight,bodyDepth), shirt);
    
    // Arms
    float armY=0.8f-lean;
    // Left arm (swings forward when right leg swings forward)
    drawPart(Vec3(-bodyWidth/2-armWidth/2,armY,0), Vec3(armWidth,armLen,armWidth), arm,
             -armSwing, 0.05f);
    // Right arm (swings opposite)
    drawPart(Vec3(bodyWidth/2+armWidth/2,armY,0), Vec3(armWidth,armLen,armWidth), arm,
             armSwing, -0.05f);
    
    // Legs
    float legY=0.15f;
    // Left leg
    drawPart(Vec3(-0.1f,legY,0), Vec3(legWidth,legLen,legWidth), pants,
             legSwing, 0);
    // Right leg (swings opposite)
    drawPart(Vec3(0.1f,legY,0), Vec3(legWidth,legLen,legWidth), pants,
             -legSwing, 0);
    
    // Shoes (darker)
    Vec3 shoe(0.15f,0.15f,0.2f);
    drawPart(Vec3(-0.1f,legY-legLen/2-0.05f,0.05f), Vec3(legWidth*1.2f,0.08f,legWidth*1.5f), shoe, legSwing, 0);
    drawPart(Vec3(0.1f,legY-legLen/2-0.05f,0.05f), Vec3(legWidth*1.2f,0.08f,legWidth*1.5f), shoe, -legSwing, 0);
}
void VoxelGame::RenderBlockHighlight(Renderer* renderer, Camera* camera) {
    if (m_wireframePipeline == kInvalidHandle || m_wireframeCubeMesh == kInvalidHandle) return;
    if (!m_raycastResult.hit) return;
    Vec3 blockPos((f32)m_raycastResult.blockX + 0.5f, (f32)m_raycastResult.blockY + 0.5f, (f32)m_raycastResult.blockZ + 0.5f);
    renderer->BindPipelineState(m_wireframePipeline);
    renderer->SetUniformMat4("u_View", camera->GetViewMatrix());
    renderer->SetUniformMat4("u_Projection", camera->GetProjectionMatrix());
    Mat4 model = glm::translate(Mat4(1.0f), blockPos);
    renderer->SetUniformMat4("u_Model", model);
    renderer->SetUniformVec4("u_Color", Vec4(0.0f, 0.0f, 0.0f, 1.0f));
    renderer->DrawMesh(m_wireframeCubeMesh);
    // Outer highlight
    model = glm::scale(glm::translate(Mat4(1.0f), blockPos), Vec3(1.02f));
    renderer->SetUniformMat4("u_Model", model);
    renderer->SetUniformVec4("u_Color", Vec4(1.0f, 1.0f, 1.0f, 0.8f));
    renderer->DrawMesh(m_wireframeCubeMesh);
}
void VoxelGame::CollectItem(BlockType type) {
    i32 idx = (i32)type;
    if (idx >= 0 && idx < kMaxInventory) {
        m_inventory[idx]++;
    }
}

bool VoxelGame::HasItem(BlockType type) const {
    i32 idx = (i32)type;
    return (idx >= 0 && idx < kMaxInventory) ? m_inventory[idx] > 0 : false;
}

void VoxelGame::ApplyFallDamage() {
    Vec3 vel = m_player.GetVelocity();
    bool onGround = m_player.IsOnGround();
    if (onGround && !m_wasOnGround) {
        f32 fallDist = m_prevY - m_player.GetPosition().y;
        if (fallDist > 3.0f) {
            i32 dmg = (i32)((fallDist - 3.0f) * 2.0f);
            if (dmg > 0) {
                m_health -= (f32)dmg;
                m_lastDamageTime = m_gameTime;
            }
        }
    }
    m_prevY = m_player.GetPosition().y;
    m_wasOnGround = onGround;
}

void VoxelGame::UpdateSurvival(f32 deltaTime) {
    // Fall damage
    ApplyFallDamage();
    // Hunger depletes over time
    if (m_player.IsMoving()) {
        m_hunger -= deltaTime * 0.02f;
        if (m_hunger < 0.0f) m_hunger = 0.0f;
    }
    // Regenerate health if hunger > 18
    if (m_hunger > 18.0f && m_health < m_maxHealth) {
        m_health += deltaTime * 0.5f;
        if (m_health > m_maxHealth) m_health = m_maxHealth;
    }
    // Starvation damage
    if (m_hunger <= 0.0f && m_gameTime - m_lastDamageTime > 2.0f) {
        m_health -= deltaTime * 1.0f;
        if (m_health <= 0.0f) {
            m_health = m_maxHealth;
            i32 th = m_world->GetTerrainGenerator().GetHeightAt(0,0);
            m_player.SetPosition(Vec3(0.0f, (f32)(th+3), 0.0f));
        }
    }
}

void VoxelGame::RenderCraftingScreen(Renderer* renderer, f32 winW, f32 winH) {
    if (m_uiPipeline == kInvalidHandle || m_whiteQuadMesh == kInvalidHandle) return;
    renderer->BindPipelineState(m_uiPipeline);
    Mat4 ortho = glm::ortho(0.0f, winW, 0.0f, winH, -1.0f, 1.0f);
    renderer->SetUniformMat4("u_Projection", ortho);
    
    // Dark overlay
    renderer->SetUniformVec4("u_Color", Vec4(0.0f, 0.0f, 0.0f, 0.6f));
    Mat4 bg = glm::scale(Mat4(1.0f), Vec3(winW, winH, 1.0f));
    renderer->SetUniformMat4("u_Model", bg);
    renderer->DrawMesh(m_whiteQuadMesh);
    
    // Crafting recipes grid
    f32 cx = winW * 0.1f, cy = winH * 0.7f;
    f32 slotSize = 30.0f, gap = 3.0f;
    
    // Recipe definitions: [ingredient1, result]
    struct Recipe { BlockType input; BlockType output; };
    Recipe recipes[] = {
        {BlockType::OakLog, BlockType::OakPlanks},
        {BlockType::OakPlanks, BlockType::CraftingTable},
        {BlockType::Stone, BlockType::Furnace},
        {BlockType::Cobblestone, BlockType::Stone},
    };
    int numRecipes = sizeof(recipes) / sizeof(recipes[0]);
    
    for (int i = 0; i < numRecipes; i++) {
        f32 rx = cx + (i % 4) * (slotSize * 2.5f + gap);
        f32 ry = cy - (i / 4) * (slotSize + gap + 20);
        
        // Input block
        Vec3 ic = BlockInfo::GetFaceColor(recipes[i].input, BlockFace::Top);
        renderer->SetUniformVec4("u_Color", Vec4(ic.x, ic.y, ic.z, 1.0f));
        Mat4 sm = Translate(Mat4(1.0f), Vec3(rx, ry, 0.0f));
        sm = Scale(sm, Vec3(slotSize, slotSize, 1.0f));
        renderer->SetUniformMat4("u_Model", sm);
        renderer->DrawMesh(m_whiteQuadMesh);
        
        // Arrow (colored bar)
        renderer->SetUniformVec4("u_Color", Vec4(0.6f, 0.6f, 0.6f, 0.8f));
        Mat4 am = Translate(Mat4(1.0f), Vec3(rx + slotSize + 2, ry + slotSize/3, 0.0f));
        am = Scale(am, Vec3(slotSize * 0.5f, slotSize/3, 1.0f));
        renderer->SetUniformMat4("u_Model", am);
        renderer->DrawMesh(m_whiteQuadMesh);
        
        // Result block
        Vec3 rc = BlockInfo::GetFaceColor(recipes[i].output, BlockFace::Top);
        renderer->SetUniformVec4("u_Color", Vec4(rc.x, rc.y, rc.z, 1.0f));
        sm = Translate(Mat4(1.0f), Vec3(rx + slotSize * 1.7f, ry, 0.0f));
        sm = Scale(sm, Vec3(slotSize, slotSize, 1.0f));
        renderer->SetUniformMat4("u_Model", sm);
        renderer->DrawMesh(m_whiteQuadMesh);
    }
}

void VoxelGame::RenderFurnaceScreen(Renderer* renderer, f32 winW, f32 winH) {
    if (m_uiPipeline == kInvalidHandle || m_whiteQuadMesh == kInvalidHandle) return;
    renderer->BindPipelineState(m_uiPipeline);
    Mat4 ortho = glm::ortho(0.0f, winW, 0.0f, winH, -1.0f, 1.0f);
    renderer->SetUniformMat4("u_Projection", ortho);
    
    // Dark overlay
    renderer->SetUniformVec4("u_Color", Vec4(0.0f, 0.0f, 0.0f, 0.7f));
    Mat4 bg = glm::scale(Mat4(1.0f), Vec3(winW, winH, 1.0f));
    renderer->SetUniformMat4("u_Model", bg);
    renderer->DrawMesh(m_whiteQuadMesh);
    
    // Furnace UI: input slot (top) -> arrow -> output slot (bottom)
    f32 cx = winW * 0.5f, cy = winH * 0.6f;
    f32 slotSize = 40.0f;
    
    // Input slot (top)
    renderer->SetUniformVec4("u_Color", Vec4(0.3f, 0.3f, 0.3f, 0.8f));
    Mat4 sm = Translate(Mat4(1.0f), Vec3(cx - slotSize/2, cy, 0.0f));
    sm = Scale(sm, Vec3(slotSize, slotSize, 1.0f));
    renderer->SetUniformMat4("u_Model", sm);
    renderer->DrawMesh(m_whiteQuadMesh);
    
    // Smelting arrow (progress bar)
    renderer->SetUniformVec4("u_Color", Vec4(0.8f, 0.4f, 0.1f, 0.9f));
    Mat4 am = Translate(Mat4(1.0f), Vec3(cx - slotSize/2, cy - slotSize - 5, 0.0f));
    am = Scale(am, Vec3(slotSize, 4.0f, 1.0f));
    renderer->SetUniformMat4("u_Model", am);
    renderer->DrawMesh(m_whiteQuadMesh);
    
    // Output slot (bottom)
    renderer->SetUniformVec4("u_Color", Vec4(0.3f, 0.3f, 0.3f, 0.8f));
    sm = Translate(Mat4(1.0f), Vec3(cx - slotSize/2, cy - slotSize*2 - 10, 0.0f));
    sm = Scale(sm, Vec3(slotSize, slotSize, 1.0f));
    renderer->SetUniformMat4("u_Model", sm);
    renderer->DrawMesh(m_whiteQuadMesh);
    
    // Instructions (colored bar)
    renderer->SetUniformVec4("u_Color", Vec4(0.2f, 0.6f, 0.4f, 0.7f));
    Mat4 tm = Translate(Mat4(1.0f), Vec3(cx - 60, cy + slotSize + 10, 0.0f));
    tm = Scale(tm, Vec3(120.0f, 4.0f, 1.0f));
    renderer->SetUniformMat4("u_Model", tm);
    renderer->DrawMesh(m_whiteQuadMesh);
}

void VoxelGame::SpawnFoodDrop(const Vec3& pos) {
    // Spawn food items (just conceptual - restore hunger on pickup)
    // We reuse the drop item system with a special type
    DropItem di;
    di.pos = pos + Vec3(0, 0.5f, 0);
    di.vel = Vec3((f32)((rand()%100-50)/100.0f), 0.5f, (f32)((rand()%100-50)/100.0f));
    di.life = 30.0f;
    di.blockType = BlockType::OakPlanks;  // Use planks as food visual
    di.collected = false;
    m_dropItems.push_back(di);
}

void VoxelGame::RenderInventoryScreen(Renderer* renderer, f32 winW, f32 winH) {
    if (m_uiPipeline == kInvalidHandle || m_whiteQuadMesh == kInvalidHandle) return;
    renderer->BindPipelineState(m_uiPipeline);
    Mat4 ortho = glm::ortho(0.0f, winW, 0.0f, winH, -1.0f, 1.0f);
    renderer->SetUniformMat4("u_Projection", ortho);
    
    // Dark overlay
    renderer->SetUniformVec4("u_Color", Vec4(0.0f, 0.0f, 0.0f, 0.6f));
    Mat4 bg = glm::scale(Mat4(1.0f), Vec3(winW, winH, 1.0f));
    renderer->SetUniformMat4("u_Model", bg);
    renderer->DrawMesh(m_whiteQuadMesh);
    
    // Creative inventory: show ALL block types in a grid
    // Air+Water excluded from grid
    f32 slotSize = 32.0f, gap = 3.0f;
    f32 startX = winW * 0.5f - 4.5f * (slotSize + gap);
    f32 startY = winH * 0.8f;
    f32 totalW = 9 * (slotSize + gap);
    
    // Category label area (top)
    renderer->SetUniformVec4("u_Color", Vec4(0.4f, 0.4f, 0.6f, 0.7f));
    Mat4 label = Translate(Mat4(1.0f), Vec3(startX, startY + 40, 0.0f));
    label = Scale(label, Vec3(totalW, 3.0f, 1.0f));
    renderer->SetUniformMat4("u_Model", label);
    renderer->DrawMesh(m_whiteQuadMesh);
    
    // Draw all blocks in grid
    int blockIdx = 0;
    for (int t = 1; t < (int)BlockType::NumTypes; t++) {
        BlockType bt = (BlockType)t;
        // Skip non-cube blocks
        if (bt == BlockType::Air || bt == BlockType::Water || bt == BlockType::Sticks) continue;
        
        int col = blockIdx % 9;
        int row = blockIdx / 9;
        if (row > 4) break;  // Max 5 rows (45 blocks)
        blockIdx++;
        
        f32 sx = startX + col * (slotSize + gap);
        f32 sy = startY - row * (slotSize + gap);
        
        // Slot background
        renderer->SetUniformVec4("u_Color", Vec4(0.2f, 0.2f, 0.2f, 0.7f));
        Mat4 sm = Translate(Mat4(1.0f), Vec3(sx, sy, 0.0f));
        sm = Scale(sm, Vec3(slotSize, slotSize, 1.0f));
        renderer->SetUniformMat4("u_Model", sm);
        renderer->DrawMesh(m_whiteQuadMesh);
        
        // Block color preview
        Vec3 bc = BlockInfo::GetFaceColor(bt, BlockFace::Top);
        if (bt == BlockType::OakLog || bt == BlockType::CraftingTable) 
            bc = BlockInfo::GetFaceColor(bt, BlockFace::Left);
        else if (bt == BlockType::Furnace || bt == BlockType::FurnaceLit) 
            bc = BlockInfo::GetFaceColor(bt, BlockFace::Top);
        renderer->SetUniformVec4("u_Color", Vec4(bc.x, bc.y, bc.z, 1.0f));
        sm = Translate(Mat4(1.0f), Vec3(sx + 3, sy + 3, 0.0f));
        sm = Scale(sm, Vec3(slotSize - 6, slotSize - 6, 1.0f));
        renderer->SetUniformMat4("u_Model", sm);
        renderer->DrawMesh(m_whiteQuadMesh);
        
        // Selection indicator for current hotbar item
        for (int h = 0; h < 9; h++) {
            if (GetSelectedBlock() == bt && h == m_selectedSlot) {
                renderer->SetUniformVec4("u_Color", Vec4(1.0f, 1.0f, 1.0f, 0.5f));
                Mat4 hm = Translate(Mat4(1.0f), Vec3(sx - 1, sy - 1, 0.0f));
                hm = Scale(hm, Vec3(slotSize + 2, slotSize + 2, 1.0f));
                renderer->SetUniformMat4("u_Model", hm);
                renderer->DrawMesh(m_whiteQuadMesh);
            }
        }
    }
    
    // Click handling: left-click on a block selects it for hotbar
    // This is handled in HandleBlockInteraction when inventory is open
}

void VoxelGame::SpawnBlockDrop(const Vec3& pos, BlockType type) {
    DropItem di;
    di.pos = pos;
    di.vel = Vec3((f32)((rand()%100-50)/100.0f), (f32)(rand()%100)/50.0f+0.3f, (f32)((rand()%100-50)/100.0f));
    di.life = 20.0f;
    di.blockType = type;
    di.collected = false;
    m_dropItems.push_back(di);
}

void VoxelGame::UpdateDrops(f32 deltaTime) {
    Vec3 ppos = m_player.GetPosition();
    for (auto it = m_dropItems.begin(); it != m_dropItems.end(); ) {
        it->life -= deltaTime;
        // Simple gravity
        it->vel.y -= 15.0f * deltaTime;
        it->pos += it->vel * deltaTime;
        // Ground collision
        if (it->pos.y < 0.0f) { it->pos.y = 0.0f; it->vel.y *= -0.3f; }
        // Check if player picks it up
        Vec3 diff = it->pos - ppos;
        float dist = sqrtf(diff.x*diff.x + diff.y*diff.y + diff.z*diff.z);
        if (dist < 1.5f && !it->collected) {
            it->collected = true;
            CollectItem(it->blockType);
            // Food items (from mob drops) restore hunger
            if (it->blockType == BlockType::OakPlanks) {
                m_hunger += 6.0f;
                if (m_hunger > 20.0f) m_hunger = 20.0f;
            }
        }
        // Remove if expired or collected
        if (it->life <= 0.0f || it->collected) {
            it = m_dropItems.erase(it);
        } else {
            ++it;
        }
    }
}

void VoxelGame::RenderDrops(Renderer* renderer, Camera* camera) {
    if (m_dropItemMesh == kInvalidHandle || m_worldPipeline == kInvalidHandle) return;
    renderer->BindPipelineState(m_worldPipeline);
    renderer->SetUniformMat4("u_View", camera->GetViewMatrix());
    renderer->SetUniformMat4("u_Projection", camera->GetProjectionMatrix());
    renderer->SetUniformVec3("u_CameraPos", camera->GetPosition());
    float sunAngle = m_gameTime * 0.0005f;
    Vec3 ld = glm::normalize(Vec3(cosf(sunAngle)*0.8f, -sinf(sunAngle)*0.8f-0.2f, sinf(sunAngle*0.7f)*0.5f));
    if (ld.y > 0.0f) ld.y = -0.1f;
    renderer->SetUniformVec3("u_LightDir", ld);
    float dayFactor = sinf(sunAngle)*0.5f+0.5f;
    renderer->SetUniformVec3("u_LightColor", Vec3(1.0f,0.95f,0.85f)*(0.3f+dayFactor*0.9f));
    renderer->SetUniformFloat("u_Time", m_gameTime);
    renderer->SetUniformFloat("u_AtlasGridX", 6.0f);
    renderer->SetUniformFloat("u_AtlasGridY", 4.0f);
    if (m_blockAtlas != kInvalidHandle) {
        renderer->BindTexture(m_blockAtlas, 0);
        renderer->SetUniformInt("u_BlockAtlas", 0);
    }
    for (auto& di : m_dropItems) {
        f32 tu = (f32)(u8)di.blockType / 255.0f;
        renderer->SetUniformFloat("u_BlockAtlasCell", tu);
        Mat4 m = glm::translate(Mat4(1.0f), di.pos);
        m = glm::scale(m, Vec3(0.3f));
        renderer->SetUniformMat4("u_Model", m);
        renderer->DrawMesh(m_dropItemMesh);
    }
}

f32 VoxelGame::GetBreakMultiplier() const {
    // Tool multiplier: certain blocks break faster with matching tool
    BlockType held = GetSelectedBlock();
    // Pickaxe types break stone/cobblestone/ore faster
    if (held == BlockType::Cobblestone || held == BlockType::Stone) return 0.4f;
    // Axe types break wood faster
    if (held == BlockType::OakPlanks || held == BlockType::OakLog) return 0.4f;
    // Shovel types break dirt/sand faster
    if (held == BlockType::Dirt || held == BlockType::Sand || held == BlockType::Grass) return 0.5f;
    return 1.0f; // Default speed
}

void VoxelGame::SpawnHostileMob(const Vec3& pos, MobType type) {
    MobEntity e;
    e.pos = pos;
    e.vel = Vec3(0,0,0);
    e.mobType = type;
    e.health = 20.0f;
    e.attackTimer = 0.0f;
    e.phase = (float)(rand() % 100) / 50.0f;
    if (type == MobType::Zombie) { e.scale=0.9f; e.r=0.2f; e.g=0.4f; e.b=0.2f; }
    else if (type == MobType::Skeleton) { e.scale=0.9f; e.r=0.7f; e.g=0.7f; e.b=0.7f; }
    else if (type == MobType::Creeper) { e.scale=0.8f; e.r=0.2f; e.g=0.7f; e.b=0.2f; }
    else { e.scale=0.6f+(float)(rand()%100)/200.0f; float h2=(float)(rand()%100)/100.0f;
        if(h2<0.33f){e.r=0.8f;e.g=0.4f;e.b=0.3f;}else if(h2<0.66f){e.r=0.9f;e.g=0.85f;e.b=0.7f;}else{e.r=0.7f;e.g=0.7f;e.b=0.7f;} }
    m_mobEntities.push_back(e);
}

void VoxelGame::UpdateMobs(f32 deltaTime) {
    Vec3 ppos = m_player.GetPosition();
    
    // Spawn initial passive mobs if none
    if (m_mobEntities.empty()) {
        for (int i = 0; i < 6; i++) {
            Vec3 sp((f32)((rand()%40)-20), 50.0f, (f32)((rand()%40)-20));
            sp.y = (f32)m_world->GetTerrainGenerator().GetHeightAt((i32)sp.x, (i32)sp.z) + 1;
            SpawnHostileMob(sp, MobType::Passive);
        }
    }
    
    // Night-time hostile mob spawning
    float dayFactor = sinf(m_gameTime * 0.0005f) * 0.5f + 0.5f;
    m_mobSpawnTimer += deltaTime;
    if (dayFactor < 0.3f && m_mobSpawnTimer > 3.0f) {  // Night time
        m_mobSpawnTimer = 0.0f;
        // Count existing hostiles
        int hostileCount = 0;
        for (auto& e : m_mobEntities) if (e.mobType != MobType::Passive) hostileCount++;
        if (hostileCount < 6) {
            Vec3 sp = ppos + Vec3((f32)((rand()%40)-20), 0, (f32)((rand()%40)-20));
            sp.y = (f32)m_world->GetTerrainGenerator().GetHeightAt((i32)sp.x, (i32)sp.z) + 1;
            MobType mt = (MobType)((rand() % 3) + 1); // Zombie, Skeleton, Creeper
            SpawnHostileMob(sp, mt);
        }
    }
    
    // Update all mobs
    for (auto it = m_mobEntities.begin(); it != m_mobEntities.end(); ) {
        it->phase += deltaTime;
        it->attackTimer -= deltaTime;
        
        Vec3 toPlayer = ppos - it->pos;
        float dist = sqrtf(toPlayer.x*toPlayer.x + toPlayer.z*toPlayer.z);
        
        if (it->mobType == MobType::Passive) {
            // Wandering behavior
            it->pos.x += sinf(it->phase * 0.7f) * deltaTime * 0.3f;
            it->pos.z += cosf(it->phase * 0.5f) * deltaTime * 0.3f;
        } else {
            // Hostile mob - chase player
            if (dist < 15.0f) {
                Vec3 dir = toPlayer / (dist + 0.001f);
                it->pos += Vec3(dir.x, 0, dir.z) * deltaTime * 2.0f;
                // Face the player
                it->phase = atan2f(dir.x, dir.z);
            }
            // Attack player on contact
            if (dist < 1.5f && it->attackTimer <= 0.0f) {
                m_health -= 2.0f;
                m_lastDamageTime = m_gameTime;
                it->attackTimer = 1.0f;
                if (m_health <= 0.0f) {
                    m_health = m_maxHealth;
                    // Respawn
                    i32 th = m_world->GetTerrainGenerator().GetHeightAt(0,0);
                    m_player.SetPosition(Vec3(0.0f, (f32)(th+3), 0.0f));
                }
            }
        }
        
        // Ground clamp
        i32 height = 50;
        if (m_world) height = m_world->GetTerrainGenerator().GetHeightAt((i32)it->pos.x, (i32)it->pos.z);
        it->pos.y = (f32)(height) + (it->mobType == MobType::Passive ? 0.5f : 0.0f);
        
        // Remove dead mobs (drop food for passive mobs)
        if (it->health <= 0.0f) {
            if (it->mobType == MobType::Passive) {
                SpawnFoodDrop(it->pos);
            }
            it = m_mobEntities.erase(it);
        } else {
            ++it;
        }
    }
}

void VoxelGame::RenderMobs(Renderer* renderer, Camera* camera, f32 deltaTime) {
    if (m_mobPipeline == kInvalidHandle) return;
    (void)deltaTime;
    
    renderer->BindPipelineState(m_mobPipeline);
    renderer->SetUniformMat4("u_View", camera->GetViewMatrix());
    renderer->SetUniformMat4("u_Projection", camera->GetProjectionMatrix());
    
    for (auto& e : m_mobEntities) {
        Vec3 pos = e.pos;
        bool hostile = e.mobType != MobType::Passive;
        
        // Body
        Mat4 m = glm::translate(Mat4(1.0f), pos);
        m = glm::scale(m, Vec3(e.scale, e.scale * 0.8f, e.scale * 0.5f));
        renderer->SetUniformMat4("u_Model", m);
        renderer->SetUniformVec4("u_Color", Vec4(e.r, e.g, e.b, 1.0f));
        renderer->DrawMesh(m_playerCubeMesh);
        
        if (hostile) {
            // Eyes (white dots on face)
            Vec3 eyePos = pos + Vec3(0, e.scale * 0.3f, -e.scale * 0.3f);
            m = glm::translate(Mat4(1.0f), eyePos);
            m = glm::scale(m, Vec3(0.08f, 0.08f, 0.02f));
            renderer->SetUniformMat4("u_Model", m);
            renderer->SetUniformVec4("u_Color", Vec4(1.0f, 0.0f, 0.0f, 1.0f));  // Red eyes
            renderer->DrawMesh(m_playerCubeMesh);
        }
        
        // Head
        float headY = pos.y + e.scale * (hostile ? 0.4f : 0.5f);
        float hs = e.scale * (hostile ? 0.5f : 0.6f);
        m = glm::translate(Mat4(1.0f), Vec3(pos.x, headY, pos.z));
        m = glm::scale(m, Vec3(hs, e.scale * 0.5f, hs));
        renderer->SetUniformMat4("u_Model", m);
        renderer->SetUniformVec4("u_Color", Vec4(e.r * 0.8f, e.g * 0.8f, e.b * 0.8f, 1.0f));
        renderer->DrawMesh(m_playerCubeMesh);
    }
}

void VoxelGame::Shutdown(Engine* engine) {
    // Save world to disk
    if (m_world) {
        _mkdir("saves");
        m_world->SaveWorld("saves/world.sav");
    }
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
        if (m_wireframeCubeMesh != kInvalidHandle) r->DestroyMesh(m_wireframeCubeMesh);
        if (m_wireframePipeline != kInvalidHandle) r->DestroyPipelineState(m_wireframePipeline);
        if (m_wireframeVS != kInvalidHandle) r->DestroyShader(m_wireframeVS);
        if (m_wireframeFS != kInvalidHandle) r->DestroyShader(m_wireframeFS);
        if (m_playerCubeMesh != kInvalidHandle) r->DestroyMesh(m_playerCubeMesh);
        if (m_playerPipeline != kInvalidHandle) r->DestroyPipelineState(m_playerPipeline);
        if (m_playerVS != kInvalidHandle) r->DestroyShader(m_playerVS);
        if (m_playerFS != kInvalidHandle) r->DestroyShader(m_playerFS);
        if (m_mobPipeline != kInvalidHandle) r->DestroyPipelineState(m_mobPipeline);
        if (m_mobVS != kInvalidHandle) r->DestroyShader(m_mobVS);
        if (m_mobFS != kInvalidHandle) r->DestroyShader(m_mobFS);
        }
    }
    m_particleSystem.Shutdown();
    m_networkServer.Stop();
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


