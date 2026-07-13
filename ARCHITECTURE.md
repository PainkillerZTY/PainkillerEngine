# Painkiller Engine Architecture

## Overview
Painkiller Engine is a modular C++17 game engine for a Minecraft-like voxel sandbox.
The engine is built as a static library (painkiller_engine) linked with the game executable.

## Module Dependency Graph
`
painkiller.exe
  └── painkiller_engine (static lib)
       ├── core      → Types.h (i8/i32/f32/Handle enums), Logger
       ├── math      → Vector.h, Matrix.h, Quaternion.h (GLM wrappers)
       ├── render    → Renderer (abstract) → OpenGLRenderer (concrete)
       ├── scene     → Scene → Entity, Camera, Components
       ├── resource  → TextureLoader (stb_image), ModelLoader, ResourceManager
       ├── engine    → Window (Win32), Input, Timer, Engine (game loop)
       └── game      → VoxelGame (orchestrator), World, Chunk, PlayerController
`

## Build System
- CMake 3.20+, C++17, MinGW GCC 6.3+ or MSVC
- Ninja build system (via setup_deps.ps1)
- Options: PAINKILLER_BUILD_GAME (ON), PAINKILLER_BUILD_DEMO (OFF), PAINKILLER_BUILD_TESTS (OFF)

## Engine Core (src/engine/)

### Engine (Engine.h)
Central game loop orchestrator:
1. Initialize() — Window + Renderer + Input + callbacks
2. Run() — Main loop: Process Input → Update → Render → Present
3. Lifecycle callbacks: SetOnInit, SetOnUpdate, SetOnRender, SetOnShutdown

Game FPS ~60, deltaTime capped at 0.05s.

### Window (Window.h/cpp)
Win32 window with:
- Raw mouse input (WM_INPUT) for hardware deltas
- ClipCursor for FPS mouse confinement
- WM_ACTIVATE handler for focus-aware cursor
- Input event forwarding to Engine::Input

### Input (Input.h/cpp)
Tracks keyboard (256 keys) + mouse (8 buttons) state per-frame.
Provides: IsKeyDown/Pressed/Released, IsMouseDown/Pressed, GetScrollDelta, GetMouseDeltaX/Y

## Render System (src/render/)

### Architecture
Abstract Renderer class → OpenGL 3.3 Core implementation.
All GPU resources tracked by opaque handle types (ShaderHandle, PipelineHandle, etc.).

### Vertex Layout
World meshes use 8-float vertex (32 bytes):
| Offset | Size | Attribute | Description |
|--------|------|-----------|-------------|
| 0 | 12 bytes | POSITION (vec3) | World-space position |
| 12 | 12 bytes | NORMAL (vec3) | Face normal |
| 24 | 8 bytes | TEXCOORD (vec2) | (blockType/255, faceIndex/6) |

### Shaders
Compiled at runtime from embedded GLSL string constants in VoxelGame.cpp.
Three pipelines:
- **World** — 3D blocks, texture atlas, lighting, fog
- **UI** — 2D orthographic overlay (crosshair, hotbar)
- **Skybox** — Procedural gradient sky

### Texture Atlas
- Single 64x48 RGBA texture (4 columns × 3 rows = 12 cells)
- Each cell 16x16 pixels
- Generated procedurally in C++ with smooth noise
- Cell mapping: 0=grass_top, 1=grass_side, 2=dirt, 3=stone, 4=cobblestone,
                 5=oak_log_top, 6=oak_log, 7=sand, 8=snow, 9=bedrock

## Scene System (src/scene/)
Entity-component architecture:
- Scene → owns Entities (id, name, vector<Component*>)
- Camera — perspective projection, view matrix, dynamic FOV
- Components: Transform, Camera, Light, Mesh, Material

Currently used minimally (mostly Camera for game).

## Game Module (src/game/)

### VoxelGame (VoxelGame.h/cpp) — 600+ lines
Main orchestrator registered as Engine callbacks.
Responsibilities:
- World creation/chunk management
- Shader/pipeline setup
- Block interaction (break/place via raycast)
- Water physics simulation
- Player controller, particles, sound
- UI rendering (crosshair, hotbar, held block, skybox)

### World (World.h/cpp)
Manages Chunk objects in hash map keyed by ChunkPos (x,z).
- Loads/unloads chunks around player (radius=4 chunks)
- Batch block updates for water physics (SetBlocksBatch)
- Frustum culling during rendering
- Water flow simulation (0.15s interval, 3-chunk radius)

### Chunk (Chunk.h/cpp)
16×128×16 block storage = 32,768 blocks per chunk.
Mesh generation: iterates blocks, checks 6 neighbors, adds face vertices for exposed sides.
Face visibility (ShouldRenderFace): only render face if neighbor is Air/Water/Leaves.

### Block (Block.h/cpp)
13 block types defined in BlockType enum.
BlockInfo table with: name, top/side/bottom colors, solid, transparent, hardness.
Static methods: Get(), GetFaceColor(), IsSolid(), IsTransparent(), IsOpaque(), GetHardness().

### Terrain Generation (TerrainGenerator.h/cpp)
Multi-octave Simplex noise for elevation + detail + biome.
4 biomes: Plains, Forest, Desert, Snowy (determined by temp/moisture).
3D noise cave carving. Tree placement with stored height (prevents floating).

### Tree Generator (TreeGenerator.h/cpp)
Oak trees: trunk (4-7 blocks) + 3-layer canopy (5×5 rounded + 3×3 + top cap).
PlaceTree sets OakLog trunk + Leaves canopy.

### PlayerController (PlayerController.h/cpp)
First-person movement with WASD, mouse look, gravity, jumping.
AABB collision detection (per-axis: X, Y, Z separately).
View bobbing, sprint/sneak, flight mode.

### BlockRaycast (BlockRaycast.h/cpp)
DDA algorithm for voxel traversal.
Ray from eye position + camera direction, max distance 8.0 blocks.
Returns: hit block position, face direction, placement position.

### ParticleSystem (ParticleSystem.h/cpp)
Block-break particles: cube mesh + velocity + gravity + fade.
SpawnBreakParticles() at block position with block color.

## Third-Party
- **GLM** — 	hird_party/glm/ — Vector, matrix, noise math
- **stb_image** — 	hird_party/stb/ — PNG/JPG loading
- **Ninja** — uild_deps/ninja/ — Build system

## Advanced Features (Planned)
See ROADMAP.md for full list.
