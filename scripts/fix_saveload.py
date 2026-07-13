import os

# 1. Chunk.h - Add raw block data access
h=open("G:/AI_projects/GameEngine/src/game/Chunk.h","r",encoding="utf-8").read()
old="    // Has geometry?\n    bool HasMesh() const { return m_hasMesh; }"
new="""    // Has geometry?
    bool HasMesh() const { return m_hasMesh; }
    // Raw block data access for save/load
    const u8* GetRawBlockData() const { return reinterpret_cast<const u8*>(m_blocks.data()); }
    void SetRawBlockData(const u8* data) { memcpy(m_blocks.data(), data, CHUNK_VOLUME); }"""
h=h.replace(old,new,1)
open("G:/AI_projects/GameEngine/src/game/Chunk.h","w",encoding="utf-8").write(h)
print("1. Chunk.h updated")

# 2. World.h - Add SaveWorld/LoadWorld declarations
h=open("G:/AI_projects/GameEngine/src/game/World.h","r",encoding="utf-8").read()
old="    // Get all loaded chunks\n    std::vector<Chunk*> GetAllChunks();"
new="""    // Get all loaded chunks
    std::vector<Chunk*> GetAllChunks();
    // Save/Load world to binary file
    bool SaveWorld(const char* path);
    bool LoadWorld(const char* path);"""
h=h.replace(old,new,1)
open("G:/AI_projects/GameEngine/src/game/World.h","w",encoding="utf-8").write(h)
print("2. World.h updated")

# 3. World.cpp - Add SaveWorld/LoadWorld implementations
cpp=open("G:/AI_projects/GameEngine/src/game/World.cpp","r",encoding="utf-8").read()
old="std::vector<Chunk*> World::GetAllChunks() {"
new="""bool World::SaveWorld(const char* path) {
    FILE* f = fopen(path, "wb");
    if (!f) return false;
    u32 magic = 0x4E494150; u32 version = 1; u32 seed = m_seed;
    fwrite(&magic,4,1,f); fwrite(&version,4,1,f); fwrite(&seed,4,1,f);
    auto chunks = GetAllChunks(); u32 n = (u32)chunks.size(); fwrite(&n,4,1,f);
    for (auto* c : chunks) {
        i32 cx=c->GetChunkX(), cz=c->GetChunkZ();
        fwrite(&cx,4,1,f); fwrite(&cz,4,1,f);
        fwrite(c->GetRawBlockData(),1,CHUNK_VOLUME,f);
    }
    fclose(f); LOG_INFO("World saved: {} chunks", n); return true;
}
bool World::LoadWorld(const char* path) {
    FILE* f = fopen(path, "rb");
    if (!f) return false;
    u32 magic,version,seed,n; fread(&magic,4,1,f);
    if (magic != 0x4E494150) { fclose(f); return false; }
    fread(&version,4,1,f); fread(&seed,4,1,f); m_seed=seed; m_terrainGen.SetSeed(seed);
    fread(&n,4,1,f);
    for (u32 i=0;i<n;i++) {
        i32 cx,cz; fread(&cx,4,1,f); fread(&cz,4,1,f);
        auto ch=std::make_unique<Chunk>(cx,cz);
        std::vector<u8> d(CHUNK_VOLUME); fread(d.data(),1,CHUNK_VOLUME,f);
        ch->SetRawBlockData(d.data()); ch->GenerateMesh(cx,cz);
        ChunkPos p(cx,cz); Chunk* ptr=ch.get();
        m_chunks[p]=std::move(ch); UploadChunkMesh(ptr);
    }
    fclose(f); LOG_INFO("World loaded: {} chunks", n); return true;
}
std::vector<Chunk*> World::GetAllChunks() {"""
cpp=cpp.replace(old,new,1)

# Add #include <cstdio> for FILE/fopen
cpp=cpp.replace('#include "World.h"','#include "World.h"\n#include <cstdio>\n#include <cstring>',1)
open("G:/AI_projects/GameEngine/src/game/World.cpp","w",encoding="utf-8").write(cpp)
print("3. World.cpp updated")

# 4. VoxelGame.cpp - Add save on shutdown, load on init
vg=open("G:/AI_projects/GameEngine/src/game/VoxelGame.cpp","r",encoding="utf-8").read()

# Save on Shutdown
old_sd='void VoxelGame::Shutdown(Engine* engine) {'
new_sd='''void VoxelGame::Shutdown(Engine* engine) {
    // Save world to disk
    if (m_world) {
        _mkdir("saves");
        m_world->SaveWorld("saves/world.sav");
    }'''
vg=vg.replace(old_sd,new_sd,1)

# Load on Initialize (after world creation, before mesh creation)
old_init='''    // 6. Load initial chunks
    UpdateChunks(engine);'''
new_init='''    // 6. Load initial chunks (try save file first)
    if (!m_world->LoadWorld("saves/world.sav")) {
        UpdateChunks(engine);
        LOG_INFO("No save found, generating new world");
    } else {
        LOG_INFO("World loaded from save");
    }'''
vg=vg.replace(old_init,new_init,1)

open("G:/AI_projects/GameEngine/src/game/VoxelGame.cpp","w",encoding="utf-8").write(vg)
print("4. VoxelGame.cpp updated")
print("All changes applied!")
