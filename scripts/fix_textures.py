import re
with open("G:/AI_projects/GameEngine/src/game/VoxelGame.cpp","r",encoding="utf-8") as f: c = f.read()
i = c.find("// 7. Build block texture atlas")
j = c.find("// 8. Crosshair mesh", i)

cpp = """    // 7. Generate procedural textures
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
    }"""

new_section = "        \n" + cpp.strip() + "\n        "
result = c[:i] + new_section + c[j:]
with open("G:/AI_projects/GameEngine/src/game/VoxelGame.cpp","w",encoding="utf-8") as f: f.write(result.replace("\\ufeff",""))
print("Atlas replaced!")


