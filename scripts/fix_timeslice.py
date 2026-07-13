# Modify World.h
h=open("G:/AI_projects/GameEngine/src/game/World.h","r",encoding="utf-8").read()

# Add ProcessChunkLoadQueue declaration after UpdateChunksAround
old="    void UpdateChunksAround(i32 centerBlockX, i32 centerBlockZ, i32 radius = 6);"
new="""    void UpdateChunksAround(i32 centerBlockX, i32 centerBlockZ, i32 radius = 6);
    void ProcessChunkLoadQueue();"""
h=h.replace(old,new,1)

# Add m_pendingLoadQueue in private section (after m_waterTimer)
old="    f32 m_waterTimer = 0.0f;\n    static constexpr f32 kWaterUpdateInterval = 0.15f;"
new="""    f32 m_waterTimer = 0.0f;
    static constexpr f32 kWaterUpdateInterval = 0.15f;
    std::vector<ChunkPos> m_pendingLoadQueue;"""
h=h.replace(old,new,1)
open("G:/AI_projects/GameEngine/src/game/World.h","w",encoding="utf-8").write(h)
print("1. World.h updated")

# Modify World.cpp
cpp=open("G:/AI_projects/GameEngine/src/game/World.cpp","r",encoding="utf-8").read()

# Modify UpdateChunksAround to queue instead of load directly
old_loop="            if (!IsChunkLoaded(cx, cz)) {\n                LoadChunk(cx, cz);\n            }"
new_loop="""            if (!IsChunkLoaded(cx, cz)) {
                bool q=false; for(auto& qp:m_pendingLoadQueue){if(qp.x==cx&&qp.z==cz){q=true;break;}}
                if(!q) m_pendingLoadQueue.push_back(ChunkPos(cx,cz));
            }"""
cpp=cpp.replace(old_loop,new_loop,1)

# Add ProcessChunkLoadQueue implementation
old_getall="std::vector<Chunk*> World::GetAllChunks() {"
new_func="""void World::ProcessChunkLoadQueue() {
    if(m_pendingLoadQueue.empty()) return;
    // Process up to 2 chunks per frame for faster loading
    for(int i=0;i<2&&!m_pendingLoadQueue.empty();i++){
        ChunkPos pos=m_pendingLoadQueue.back(); m_pendingLoadQueue.pop_back();
        if(!IsChunkLoaded(pos.x,pos.z)) LoadChunk(pos.x,pos.z);
    }
}
std::vector<Chunk*> World::GetAllChunks() {"""
cpp=cpp.replace(old_getall,new_func,1)
open("G:/AI_projects/GameEngine/src/game/World.cpp","w",encoding="utf-8").write(cpp)
print("2. World.cpp updated")

# Modify VoxelGame.cpp - add ProcessChunkLoadQueue call in Update
vg=open("G:/AI_projects/GameEngine/src/game/VoxelGame.cpp","r",encoding="utf-8").read()
old="     UpdateChunks(engine);\n     HandleBlockInteraction(engine, deltaTime);"
new="""     UpdateChunks(engine);
     m_world->ProcessChunkLoadQueue();
     HandleBlockInteraction(engine, deltaTime);"""
vg=vg.replace(old,new,1)
open("G:/AI_projects/GameEngine/src/game/VoxelGame.cpp","w",encoding="utf-8").write(vg)
print("3. VoxelGame.cpp updated")
print("All done!")
