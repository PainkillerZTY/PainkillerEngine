# Advanced Features Implementation Guide

This document explains how to implement the major architectural features
planned for the Painkiller Engine. Each section covers design, approach,
and step-by-step implementation.

---

## 1. Instanced Rendering

### Current State
Each chunk generates a separate mesh and is rendered as a separate draw call.
For 9×9 chunks (81), this means 81+ draw calls per frame.

### Target State
All visible blocks are collected per texture type and rendered with
glDrawElementsInstanced — one draw call per texture type (10 total).

### Reference
CreationEngine uses CubeModel class with:
- push(x,y,z) — adds block position to instance vector
- draw() — renders all instances with glDrawElementsInstanced
- Separate VAO for horizontal (top/bottom) and vertical (sides) faces

### Implementation Steps

#### Step 1: Add instanced draw support to OpenGLRenderer
`cpp
// In OpenGLRenderer.h add:
void DrawInstanced(MeshHandle mesh, u32 instanceCount);
// In OpenGLRenderer.cpp:
void OpenGLRenderer::DrawInstanced(MeshHandle h, u32 instanceCount) {
    auto& me = m_meshes[h];
    pfnBindVertexArray(me.vaoHandle);
    pfnDrawElementsInstanced(GL_TRIANGLES, me.indexCount, GL_UNSIGNED_INT, 0, instanceCount);
}
// Need: pfnDrawElementsInstanced function pointer
`

#### Step 2: Create instance buffer per block type
In VoxelGame, maintain std::vector<glm::mat4> per texture index (0-9):
`cpp
std::vector<glm::mat4> m_chunkInstances[10];
`

#### Step 3: Collect visible instances per frame

#### Step 4: Render each texture group with instanced draw

### Key Files to Create/Modify
- src/render/OpenGLRenderer.h/cpp — Add DrawInstanced
- src/game/VoxelGame.h/cpp — Instance collection + rendering
- src/game/Chunk.h/cpp — Per-texture face extraction

---

## 2. Player Model

### Current State
Only a held block (cube) is rendered in first person.
No player body/arms/legs.

### Target State
Full player model with head, body, arms, legs + walking animation.

### Reference Data
PersonVertexData.h from CreationEngine has head[6][48], ody[6][48],
rm[6][48], leg[6][48] vertex arrays (6 faces × 6 vertices × 5 floats each).

### Implementation Steps

#### Step 1: Copy vertex data
Create src/game/PlayerModelData.h with the vertex arrays from PersonVertexData.h.
Each part has 6 faces (front, left, back, right, bottom, top),
6 vertices per face (2 triangles), 5 floats per vertex (pos3 + uv2).

#### Step 2: Create PlayerModel class
`cpp
// src/game/PlayerModel.h
class PlayerModel {
    void Init(Renderer* r);
    void Render(Renderer* r, Camera* c, Vec3 pos, float yaw, float limbAngle);
    void Shutdown();
private:
    MeshHandle m_headMesh, m_bodyMesh, m_armMeshes[2], m_legMeshes[2];
    ShaderHandle m_vs, m_fs;
    PipelineHandle m_pipeline;
    TextureHandle m_texture;
};
`

#### Step 3: Load player texture
Use reference project's 
esource/Person/*.png textures.

#### Step 4: Animate
Limb swinging for walking: oscillate arms/legs with sin(time).

#### Step 5: Integrate
In VoxelGame::Render(), call PlayerModel::Render() after world.

### Key Files to Create/Modify
- src/game/PlayerModelData.h — Vertex data arrays
- src/game/PlayerModel.h/cpp — Player model class
- src/game/VoxelGame.h/cpp — Integration

---

## 3. Multi-Threaded Terrain Generation

### Current State
Terrain generation runs on the main thread, causing frame drops when
loading new chunks.

### Target State
Background thread pool generates chunks while main thread renders.

### Implementation Steps

#### Step 1: Create thread pool
`cpp
// src/game/ThreadPool.h
class ThreadPool {
    ThreadPool(int numThreads);
    void Enqueue(std::function<void()> task);
    void WaitAll();
private:
    std::vector<std::thread> m_threads;
    std::queue<std::function<void()>> m_tasks;
    std::mutex m_mutex;
    std::condition_variable m_cv;
    bool m_stop = false;
};
`

#### Step 2: Make chunk generation thread-safe
TerrainGenerator and Noise must be stateless (no mutable state) or use
thread-local copies.

#### Step 3: Queue chunk generation
In World::UpdateChunksAround(), instead of calling GenerateChunk directly,
enqueue it on the thread pool.

#### Step 4: Transfer completed chunks
Use a concurrent queue to pass generated chunks from worker threads
to the main thread for GPU upload.

### Key Files to Create/Modify
- src/game/ThreadPool.h/cpp — Thread pool
- src/game/World.h/cpp — Async chunk loading
- src/game/TerrainGenerator.h/cpp — Thread safety

---

## 4. Save/Load World

### Current State
No persistence — world is regenerated from seed each run.

### Target State
Chunks saved to disk, loaded on demand.

### Implementation

#### Approach: Region-based files (like Minecraft)
Divide world into 32×32 chunk regions. Each region file contains
compressed block data.

#### File format:
`
[Header: 4 bytes magic + 4 bytes version + 8 bytes timestamp]
[Chunk table: 1024 × (4 bytes offset + 4 bytes size)]
[Chunk data: zlib-compressed block array (32768 bytes raw)]
`

### Key Files to Create
- src/game/WorldSerializer.h/cpp — Save/load logic
- src/game/RegionFile.h/cpp — Region file I/O

---

## 5. LOD System

### Approach
Distant chunks render at lower resolution:
- Close (0-4 chunks): Full 16×128×16 detail
- Medium (4-8 chunks): 8×64×8 (2×2×2 merged blocks)
- Far (8-12 chunks): 4×32×4 (4×4×4 merged blocks)

### Implementation
- Generate LOD meshes during chunk creation
- Select LOD level based on distance from camera
- Transition with alpha blend or distance fade

---

## 6. Multiplayer

### Architecture
Client-Server with state synchronization:
- Server: authoritative world state
- Client: render + predict + send input

### Protocol
- TCP for reliable data (world loading)
- UDP for real-time updates (player positions)

### Key Components
- src/network/NetworkServer.h/cpp
- src/network/NetworkClient.h/cpp
- src/network/Packet.h — Serialization
