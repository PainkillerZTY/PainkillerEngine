import os
r = "G:/AI_projects/GameEngine/src/game"

# 1. Chunk.h - Add LOD members
h = open(r+"/Chunk.h","r",encoding="utf-8").read()
h = h.replace("bool m_meshDirty = true;",
    "bool m_meshDirty = true;\n    bool m_hasLODMesh = false;\n    std::vector<f32> m_lodVertices;\n    std::vector<u32> m_lodIndices;\n    u32 m_lodVertexCount = 0;\n    u32 m_lodIndexCount = 0;", 1)
h = h.replace("u32 GetVertexStride() const { return 8 * sizeof(f32); } // pos(3) + normal(3) + texcoord(2)",
    "u32 GetVertexStride() const { return 8 * sizeof(f32); }\n    bool HasLODMesh() const { return m_hasLODMesh; }\n    const std::vector<f32>& GetLODVertexData() const { return m_lodVertices; }\n    u32 GetLODVertexCount() const { return m_lodVertexCount; }\n    u32 GetLODIndexCount() const { return m_lodIndexCount; }\n    void GenerateLODMesh(i32 chunkX, i32 chunkZ);", 1)
open(r+"/Chunk.h","w",encoding="utf-8").write(h)
print("Chunk.h done")

# 2. Chunk.cpp - Add GenerateLODMesh
c = open(r+"/Chunk.cpp","r",encoding="utf-8").read()
lod = "\n// LOD\nvoid Chunk::GenerateLODMesh(i32 cx, i32 cz) {\n    m_lodVertices.clear(); m_lodIndices.clear();\n    m_lodVertexCount=0; m_lodIndexCount=0; m_hasLODMesh=false;\n    f32 bx=(f32)(cx*16), bz=(f32)(cz*16);\n    for(i32 y=0;y<128;y+=2) for(i32 z=0;z<16;z+=2) for(i32 x=0;x<16;x+=2){\n        BlockType mt=BlockType::Air; bool fnd=false;\n        for(i32 dy=0;dy<2&&!fnd;dy++) for(i32 dz=0;dz<2&&!fnd;dz++) for(i32 dx=0;dx<2&&!fnd;dx++){\n            BlockType b=GetBlock(x+dx,y+dy,z+dz);\n            if(b!=BlockType::Air){mt=b;fnd=true;}}\n        if(!fnd)continue;\n        Vec3 wo(bx+(f32)x,(f32)y,bz+(f32)z);\n        for(i32 f=0;f<6;f++){\n            i32 nx=x+(f==0?2:f==1?-2:0),ny=y+(f==2?2:f==3?-2:0),nz=z+(f==4?2:f==5?-2:0);\n            BlockType nb=BlockType::Air;\n            if(IsLocalCoordValid(nx,ny,nz)){\n                for(i32 dy=0;dy<2&&nb==BlockType::Air;dy++) for(i32 dz=0;dz<2&&nb==BlockType::Air;dz++) for(i32 dx=0;dx<2&&nb==BlockType::Air;dx++)\n                    nb=GetBlock(nx+dx,ny+dy,nz+dz);}\n            if(nb!=BlockType::Air)continue;\n            f32 tu=(f32)(u8)mt/255.0f, tv=(f32)f/6.0f;\n            Vec3 v0=kFaceVerts[f][0]*2.0f+wo; Vec3 v1=kFaceVerts[f][1]*2.0f+wo;\n            Vec3 v2=kFaceVerts[f][2]*2.0f+wo; Vec3 v3=kFaceVerts[f][3]*2.0f+wo;\n            Vec3 vs[4]={v0,v1,v2,v3};\n            for(i32 i=0;i<4;i++){\n                m_lodVertices.push_back(vs[i].x);m_lodVertices.push_back(vs[i].y);m_lodVertices.push_back(vs[i].z);\n                m_lodVertices.push_back(kFaceNormals[f].x);m_lodVertices.push_back(kFaceNormals[f].y);m_lodVertices.push_back(kFaceNormals[f].z);\n                m_lodVertices.push_back(tu);m_lodVertices.push_back(tv);}\n            u32 idx=m_lodVertexCount;\n            m_lodIndices.push_back(idx);m_lodIndices.push_back(idx+1);m_lodIndices.push_back(idx+2);\n            m_lodIndices.push_back(idx);m_lodIndices.push_back(idx+2);m_lodIndices.push_back(idx+3);\n            m_lodVertexCount+=4;m_lodIndexCount+=6;}}\n    m_hasLODMesh=m_lodIndexCount>0;}\n"
c = c.replace("void Chunk::AddFace(", lod + "void Chunk::AddFace(", 1)
open(r+"/Chunk.cpp","w",encoding="utf-8").write(c)
print("Chunk.cpp done")

# 3. World.h - Add LOD map
w = open(r+"/World.h","r",encoding="utf-8").read()
w = w.replace("std::unordered_map<ChunkPos, ChunkRenderData, ChunkPos::Hash> m_chunkRenderData;",
    "std::unordered_map<ChunkPos, ChunkRenderData, ChunkPos::Hash> m_chunkRenderData;\n    std::unordered_map<ChunkPos, MeshHandle, ChunkPos::Hash> m_lodChunkRenderData;", 1)
open(r+"/World.h","w",encoding="utf-8").write(w)
print("World.h done")

# 4. World.cpp - Modify UploadChunkMesh, Render, UnloadChunk, destructor
wc = open(r+"/World.cpp","r",encoding="utf-8").read()
# Upload LOD after main mesh
wc = wc.replace("if (handle != kInvalidHandle) {\n        ChunkRenderData renderData;\n        renderData.meshHandle = handle;\n        m_chunkRenderData[pos] = renderData;\n    }\n}",
    "if (handle != kInvalidHandle) {\n        ChunkRenderData renderData;\n        renderData.meshHandle = handle;\n        m_chunkRenderData[pos] = renderData;\n    }\n    // Upload LOD mesh\n    chunk->GenerateLODMesh(pos.x,pos.z);\n    if(chunk->HasLODMesh()){\n        MeshData ld; ld.vertices=chunk->GetLODVertexData();\n        ld.vertexCount=chunk->GetLODVertexCount(); ld.indexCount=chunk->GetLODIndexCount();\n        ld.vertexStride=8*sizeof(f32);\n        MeshHandle lh=m_renderer->CreateMesh(ld);\n        if(lh!=kInvalidHandle) m_lodChunkRenderData[pos]=lh;}", 1)
# Render LOD for distant chunks
wc = wc.replace("if (data.meshHandle == kInvalidHandle) continue;",
    "if (data.meshHandle == kInvalidHandle) continue;\n        f32 dist=std::sqrt((f32)(pos.x*pos.x+pos.z*pos.z));\n        if(dist>4.0f){\n            auto lit=m_lodChunkRenderData.find(pos);\n            if(lit!=m_lodChunkRenderData.end()){renderer->DrawMesh(lit->second);continue;}}", 1)
# Cleanup LOD in UnloadChunk
wc = wc.replace("void World::UnloadChunk(i32 chunkX, i32 chunkZ) {\n    ChunkPos pos(chunkX, chunkZ);\n\n    auto renderIt = m_chunkRenderData.find(pos);",
    "void World::UnloadChunk(i32 chunkX, i32 chunkZ) {\n    ChunkPos pos(chunkX, chunkZ);\n    auto lodIt=m_lodChunkRenderData.find(pos);\n    if(lodIt!=m_lodChunkRenderData.end()){\n        if(lodIt->second!=kInvalidHandle&&m_renderer) m_renderer->DestroyMesh(lodIt->second);\n        m_lodChunkRenderData.erase(lodIt);}\n    auto renderIt = m_chunkRenderData.find(pos);", 1)
# Cleanup LOD in destructor
wc = wc.replace("World::~World() {\n    // Clean up all chunk render data\n    for (auto& entry : m_chunkRenderData) {",
    "World::~World() {\n    for(auto& e:m_lodChunkRenderData){if(e.second!=kInvalidHandle&&m_renderer)m_renderer->DestroyMesh(e.second);}\n    m_lodChunkRenderData.clear();\n    // Clean up all chunk render data\n    for (auto& entry : m_chunkRenderData) {", 1)
open(r+"/World.cpp","w",encoding="utf-8").write(wc)
print("World.cpp done")
print("ALL LOD CHANGES COMPLETE!")
