# PainkillerEngine - AI Developer Handoff

## Project State (Current)

This is a Minecraft-like voxel game built with C++17 + OpenGL 3.3.
Last updated: 2026-07-16

## Git Repository
https://github.com/PainkillerZTY/PainkillerEngine

## Build Instructions
```
cd PainkillerEngine
cmake -B build -G Ninja
cmake --build build --target painkiller
./build/bin/painkiller.exe
```

## What Has Been Implemented

### Core Engine
- [x] OpenGL 3.3 Core renderer with abstract Renderer interface
- [x] Win32 window with raw mouse input
- [x] Custom shader system (embedded GLSL strings)
- [x] GLM math library (vector, matrix, quaternion)
- [x] Entity-component scene system

### World System
- [x] Chunk-based world (16x128x16 blocks per chunk)
- [x] 9 biome types (Plains, Desert, Forest, Snowy, Ocean, Swamp, Taiga, Jungle, Mountains)
- [x] Noise-driven terrain with FBM (Fractal Brownian Motion)
- [x] Cave generation with 3D noise
- [x] Ore generation (coal, iron, gold, diamond) at appropriate depths
- [x] Oak tree generation (with fixed chunk boundary issues)
- [x] Block raycasting (DDA algorithm)
- [x] Frustum culling for rendering

### Block System (24 types)
- Air, Grass, Dirt, Stone, Cobblestone, Wood, OakLog, Leaves
- OakPlanks, Sand, Water, Snow, Bedrock
- CoalOre, IronOre, GoldOre, DiamondOre
- CraftingTable, Furnace, FurnaceLit, Chest, Glass, Bookshelf, Sticks

### Gameplay
- [x] Block breaking with timer + hardness + tool multiplier
- [x] Block placement on adjacent face
- [x] 9-slot hotbar with scroll wheel + number keys
- [x] Creative inventory (E key) with block selection
- [x] Crafting table GUI (4 recipes)
- [x] Furnace smelting GUI
- [x] Bed (skip night via bedrock)
- [x] Player model with walking animation (third person)
- [x] Held block in first person

### Survival
- [x] Health bar (20 HP)
- [x] Hunger bar (20 points)
- [x] Fall damage (>3 blocks)
- [x] Food pickup restores hunger
- [x] Starvation damage
- [x] Death and respawn

### Mobs
- [x] Passive mobs (pig, cow, sheep) with wandering AI
- [x] Hostile mobs (zombie, skeleton, creeper) spawn at night
- [x] Mob chase AI (hostiles follow player)
- [x] Mob attack on contact
- [x] Food drops from passive mobs on death

### Visual
- [x] Solid colors per block type (flat colors in shader)
- [x] Day/night cycle (sky color + sun position)
- [x] Hemisphere ambient lighting + directional diffuse
- [x] Distance fog
- [x] Procedural sky with stars at night

### UI
- [x] Crosshair
- [x] Health/hunger bars
- [x] Hotbar display
- [x] F3 debug screen (XYZ, chunk, seed, facing, target block)
- [x] Creative inventory (E key)
- [x] Crafting GUI (right-click crafting table)
- [x] Furnace GUI (right-click furnace)

### Networking
- [x] TCP server on port 25565
- [x] Client accept/disconnect management
- [x] Player position broadcast
- [x] Basic packet serialization

### Performance
- [x] Greedy meshing (FIXED - no stack overflow)
- [x] Pre-allocated vertex buffers
- [x] Frustum culling
- [x] Timesliced chunk loading (2 chunks/frame)

## What Needs Work (Priority Order)

### CRITICAL - Fix these first
1. **Fix greedy meshing rendering errors** (`src/game/Chunk.cpp`)
   - The greedy meshing has been restored but may produce visual artifacts
   - Fallback: comment out greedy, use per-face generation

2. **Multi-threaded chunk generation** (`src/game/World.cpp`)
   - Use enkiTS library (already downloaded at `third_party/enkiTS.h`)
   - Generate terrain + mesh in background threads
   - main thread only handles GPU upload

3. **Block placement doesn't work** (`src/game/VoxelGame.cpp`)
   - Debug the raycast result and hotbar block initialization
   - Check GetSelectedBlock() returns correct type

### HIGH - Next priority
4. **Transparent sorting** - Render glass, water, leaves after opaque
5. **Block drops when breaking** - Item entity with physics
6. **Better crafting** - Real 2x2/3x3 grid with recipe book

### MEDIUM - Future features
7. **Weather system** (rain, snow)
8. **Redstone system** (basic wiring)
9. **Structures** (village houses, dungeons)
10. **The Nether** dimension

### Technical Debt
- `VoxelGame.cpp` is 1800+ lines - should be split into modules
- OpenGL function loading is manual - consider glad/gl3w
- No logging to file - all log goes to stderr
- CMakeLists.txt references broken enkiTS - needs cleanup
- Vector.h redefines GLM macros - causes build warnings

## Third-party Libraries Downloaded
| Library | Location | Status |
|---------|----------|--------|
| GLM (math) | third_party/glm/ | Integrated |
| stb_image | third_party/stb/ | Integrated |
| stb_truetype | third_party/stb/ | Downloaded, not integrated |
| enkiTS | third_party/enkiTS.h | Downloaded, not integrated |
| Dear ImGui | third_party/imgui/ | Downloaded, needs OpenGL loader fix |
