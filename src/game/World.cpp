#include "World.h"
#include <cstdio>
#include <cstring>
#include <tuple>
#include <cmath>
#include "Chunk.h"
#include "Logger.h"
#include <algorithm>

namespace painkiller {

World::World(Renderer* renderer)
    : m_renderer(renderer)
{
}

World::~World() {
    // Clean up all chunk render data
    for (auto& entry : m_chunkRenderData) {
        ChunkRenderData& data = entry.second;
        if (data.meshHandle != kInvalidHandle && m_renderer) {
            m_renderer->DestroyMesh(data.meshHandle);
        }
    }
    m_chunkRenderData.clear();
    m_chunks.clear();
}

void World::Initialize(u32 seed) {
    m_seed = seed;
    m_terrainGen.SetSeed(seed);
    m_initialized = true;
    LOG_INFO("World initialized with seed: {}", seed);
}

// ============================================================
// Chunk Management
// ============================================================
Chunk* World::GetChunk(i32 chunkX, i32 chunkZ) {
    auto it = m_chunks.find(ChunkPos(chunkX, chunkZ));
    return it != m_chunks.end() ? it->second.get() : nullptr;
}

bool World::IsChunkLoaded(i32 chunkX, i32 chunkZ) const {
    return m_chunks.find(ChunkPos(chunkX, chunkZ)) != m_chunks.end();
}

Chunk* World::LoadChunk(i32 chunkX, i32 chunkZ) {
    if (IsChunkLoaded(chunkX, chunkZ)) {
        return GetChunk(chunkX, chunkZ);
    }

    auto chunk = std::make_unique<Chunk>(chunkX, chunkZ);

    // Generate terrain
    m_terrainGen.GenerateChunk(chunk.get(), chunkX, chunkZ);

    // Generate mesh data
    chunk->GenerateMesh(chunkX, chunkZ);

    // Store chunk
    ChunkPos pos(chunkX, chunkZ);
    Chunk* ptr = chunk.get();
    m_chunks[pos] = std::move(chunk);

    // Upload mesh to GPU
    UploadChunkMesh(ptr);

    return ptr;
}

void World::UnloadChunk(i32 chunkX, i32 chunkZ) {
    ChunkPos pos(chunkX, chunkZ);

    auto renderIt = m_chunkRenderData.find(pos);
    if (renderIt != m_chunkRenderData.end()) {
        if (renderIt->second.meshHandle != kInvalidHandle && m_renderer) {
            m_renderer->DestroyMesh(renderIt->second.meshHandle);
        }
        m_chunkRenderData.erase(renderIt);
    }

    m_chunks.erase(pos);
}

void World::UpdateChunksAround(i32 centerBlockX, i32 centerBlockZ, i32 radius) {
    i32 centerChunkX = (centerBlockX >= 0) ? centerBlockX / CHUNK_SIZE_X :
                        (centerBlockX + 1) / CHUNK_SIZE_X - 1;
    i32 centerChunkZ = (centerBlockZ >= 0) ? centerBlockZ / CHUNK_SIZE_Z :
                        (centerBlockZ + 1) / CHUNK_SIZE_Z - 1;

    // Load chunks within radius
    for (i32 dx = -radius; dx <= radius; ++dx) {
        for (i32 dz = -radius; dz <= radius; ++dz) {
            i32 cx = centerChunkX + dx;
            i32 cz = centerChunkZ + dz;
            if (!IsChunkLoaded(cx, cz)) {
                bool q=false; for(auto& qp:m_pendingLoadQueue){if(qp.x==cx&&qp.z==cz){q=true;break;}}
                if(!q) m_pendingLoadQueue.push_back(ChunkPos(cx,cz));
            }
        }
    }

    // Unload chunks outside radius (with margin)
    // Keep a slightly larger ring for hysteresis
    const i32 unloadRadius = radius + 2;
    std::vector<ChunkPos> toUnload;
    for (auto& entry : m_chunks) {
        ChunkPos pos = entry.first;
        i32 dx = pos.x - centerChunkX;
        i32 dz = pos.z - centerChunkZ;
        if (abs(dx) > unloadRadius || abs(dz) > unloadRadius) {
            toUnload.push_back(pos);
        }
    }

    for (auto& pos : toUnload) {
        UnloadChunk(pos.x, pos.z);
    }
}

// ============================================================
// Block I/O (world coordinates)
// ============================================================
BlockType World::GetBlock(i32 worldX, i32 worldY, i32 worldZ) const {
    if (worldY < 0 || worldY >= CHUNK_SIZE_Y) return BlockType::Air;

    i32 chunkX, chunkZ, localX, localY, localZ;
    WorldToChunkCoords(worldX, worldY, worldZ, chunkX, chunkZ, localX, localY, localZ);

    auto it = m_chunks.find(ChunkPos(chunkX, chunkZ));
    if (it != m_chunks.end()) {
        return it->second->GetBlock(localX, localY, localZ);
    }
    return BlockType::Air;
}

void World::SetBlock(i32 worldX, i32 worldY, i32 worldZ, BlockType type) {
    if (worldY < 0 || worldY >= CHUNK_SIZE_Y) return;

    i32 chunkX, chunkZ, localX, localY, localZ;
    WorldToChunkCoords(worldX, worldY, worldZ, chunkX, chunkZ, localX, localY, localZ);

    auto it = m_chunks.find(ChunkPos(chunkX, chunkZ));
    if (it != m_chunks.end()) {
        it->second->SetBlock(localX, localY, localZ, type);
        it->second->GenerateMesh(chunkX, chunkZ);
        UploadChunkMesh(it->second.get());
    }
}

// ============================================================
// GPU Mesh Upload
// ============================================================
void World::UploadChunkMesh(Chunk* chunk) {
    if (!m_renderer || !chunk) return;

    ChunkPos pos(chunk->GetChunkX(), chunk->GetChunkZ());

    // Destroy old mesh
    auto renderIt = m_chunkRenderData.find(pos);
    if (renderIt != m_chunkRenderData.end()) {
        if (renderIt->second.meshHandle != kInvalidHandle) {
            m_renderer->DestroyMesh(renderIt->second.meshHandle);
        }
        m_chunkRenderData.erase(renderIt);
    }

    // Create new mesh if chunk has geometry
    if (!chunk->HasMesh()) return;

    MeshData meshData;
    meshData.vertices = chunk->GetVertexData();
    meshData.indices = chunk->GetIndexData();
    meshData.vertexCount = chunk->GetVertexCount();
    meshData.indexCount = chunk->GetIndexCount();
    meshData.vertexStride = chunk->GetVertexStride();

    MeshHandle handle = m_renderer->CreateMesh(meshData);
    if (handle != kInvalidHandle) {
        ChunkRenderData renderData;
        renderData.meshHandle = handle;
        m_chunkRenderData[pos] = renderData;
    }
}

// ============================================================
// Rendering
// ============================================================
void World::Render(Renderer* renderer, const Frustum* frustum) {
    if (!renderer) return;

    for (auto& entry : m_chunkRenderData) {
        const ChunkPos& pos = entry.first;
        ChunkRenderData& data = entry.second;
        if (data.meshHandle == kInvalidHandle) continue;
        
        // Frustum culling: check chunk AABB against frustum
        if (frustum) {
            Vec3 aabbMin((f32)(pos.x * CHUNK_SIZE_X), 0.0f, (f32)(pos.z * CHUNK_SIZE_Z));
            Vec3 aabbMax((f32)((pos.x + 1) * CHUNK_SIZE_X), (f32)CHUNK_SIZE_Y, (f32)((pos.z + 1) * CHUNK_SIZE_Z));
            if (!frustum->Intersects(aabbMin, aabbMax)) continue;
        }
        
        renderer->DrawMesh(data.meshHandle);
    }
}

void World::SetBlocksBatch(const std::vector<std::tuple<i32,i32,i32,BlockType>>& blocks) {
    // Group changes by chunk to batch mesh regeneration
    std::unordered_map<ChunkPos, std::vector<std::tuple<i32,i32,i32,BlockType>>, ChunkPos::Hash> chunkChanges;
    for (auto& t : blocks) {
        i32 wx = std::get<0>(t), wy = std::get<1>(t), wz = std::get<2>(t);
        if (wy < 0 || wy >= CHUNK_SIZE_Y) continue;
        i32 cx, cz, lx, ly, lz;
        WorldToChunkCoords(wx, wy, wz, cx, cz, lx, ly, lz);
        auto it = m_chunks.find(ChunkPos(cx, cz));
        if (it != m_chunks.end()) {
            chunkChanges[ChunkPos(cx, cz)].push_back(t);
        }
    }
    for (auto& entry : chunkChanges) {
        Chunk* chunk = m_chunks[entry.first].get();
        if (!chunk) continue;
        for (auto& t : entry.second) {
            i32 wx = std::get<0>(t), wy = std::get<1>(t), wz = std::get<2>(t);
            i32 lx = wx - entry.first.x * CHUNK_SIZE_X;
            i32 ly = wy;
            i32 lz = wz - entry.first.z * CHUNK_SIZE_Z;
            if (chunk->IsLocalCoordValid(lx, ly, lz))
                chunk->SetBlock(lx, ly, lz, std::get<3>(t));
        }
        chunk->GenerateMesh(entry.first.x, entry.first.z);
        UploadChunkMesh(chunk);
    }
}

void World::UpdateWaterPhysics(f32 deltaTime, i32 centerX, i32 centerZ) {
    m_waterTimer += deltaTime;
    if (m_waterTimer < kWaterUpdateInterval) return;
    m_waterTimer = 0.0f;
    
    i32 centerChunkX = (centerX >= 0) ? centerX / CHUNK_SIZE_X : (centerX + 1) / CHUNK_SIZE_X - 1;
    i32 centerChunkZ = (centerZ >= 0) ? centerZ / CHUNK_SIZE_Z : (centerZ + 1) / CHUNK_SIZE_Z - 1;
    
    struct WaterMove { i32 x, y, z; BlockType blockType; };
    std::vector<WaterMove> moves;
    
    // Process chunks near player for water physics
    for (i32 dx = -3; dx <= 3; ++dx) {
        for (i32 dz = -3; dz <= 3; ++dz) {
            i32 cx = centerChunkX + dx;
            i32 cz = centerChunkZ + dz;
            auto it = m_chunks.find(ChunkPos(cx, cz));
            if (it == m_chunks.end()) continue;
            
            i32 baseX = cx * CHUNK_SIZE_X;
            i32 baseZ = cz * CHUNK_SIZE_Z;
            
            // Scan columns for water (near water level + below for falling water)
            for (i32 y = std::min(55, CHUNK_SIZE_Y - 2); y >= 15; --y) {
                for (i32 z = 0; z < CHUNK_SIZE_Z; ++z) {
                    for (i32 x = 0; x < CHUNK_SIZE_X; ++x) {
                        i32 wx = baseX + x;
                        int wy = y;
                        i32 wz = baseZ + z;
                        
                        BlockType block = GetBlock(wx, wy, wz);
                        if (block != BlockType::Water) continue;
                        
                        // Flow downward
                        if (wy > 0) {
                            BlockType below = GetBlock(wx, wy - 1, wz);
                            if (below == BlockType::Air) {
                                moves.push_back({wx, wy - 1, wz, BlockType::Water});
                                moves.push_back({wx, wy, wz, BlockType::Air});
                                continue;
                            }
                        }
                        
                        // Flow horizontal
                        bool flowed = false;
                        static const int dirs[4][2] = {{1,0},{-1,0},{0,1},{0,-1}};
                        for (int d = 0; d < 4 && !flowed; ++d) {
                            i32 nx = wx + dirs[d][0];
                            i32 nz = wz + dirs[d][1];
                            BlockType adj = GetBlock(nx, wy, nz);
                            if (adj == BlockType::Air) {
                                moves.push_back({nx, wy, nz, BlockType::Water});
                                moves.push_back({wx, wy, wz, BlockType::Air});
                                flowed = true;
                            }
                        }
                    }
                }
            }
        }
    }
    
    // Apply all water moves directly
    for (auto& m : moves) {
        SetBlock(m.x, m.y, m.z, m.blockType);
    }
}

bool World::SaveWorld(const char* path) {
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
void World::ProcessChunkLoadQueue() {
    if(m_pendingLoadQueue.empty()) return;
    // Process up to 2 chunks per frame for faster loading
    for(int i=0;i<2&&!m_pendingLoadQueue.empty();i++){
        ChunkPos pos=m_pendingLoadQueue.back(); m_pendingLoadQueue.pop_back();
        if(!IsChunkLoaded(pos.x,pos.z)) LoadChunk(pos.x,pos.z);
    }
}
std::vector<Chunk*> World::GetAllChunks() {
    std::vector<Chunk*> result;
    for (auto& entry : m_chunks) { result.push_back(entry.second.get()); }
    return result;
}

} // namespace painkiller
