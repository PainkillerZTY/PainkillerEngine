# Painkiller Engine v0.1.0

## Architecture

```
+==========================================================+
|                  GAME APPLICATION LAYER                   |
|    VoxelGame, PlayerController, ParticleSystem, etc.     |
|    World, Chunk, TerrainGeneration                       |
+==========================================================+
|                   DEMO / SANDBOX                          |
|    (DemoApp, main.cpp - replaced by VoxelGame)           |
+==========================================================+
|                   SCENE / ECS LAYER                       |
|    Entity, Component, Scene, Camera                      |
|    Components: Transform, Mesh, Camera, Light, Mat.      |
+==========================================================+
|                  RESOURCE MANAGEMENT                      |
|    ResourceManager (caching, lifetime)                   |
|    ModelLoader, TextureLoader (import pipeline)          |
+==========================================================+
|                RENDER ABSTRACTION LAYER                   |
|    Renderer (pure virtual interface)                     |
|    Handles, Descriptors, PipelineState                   |
+==========================================================+
|   OpenGL 3.3 Backend    |  (Future: D3D12 / Vulkan)      |
+==========================================================+
|                    CORE / MATH LAYER                      |
|    Types, Logger, Timer, Input, Window                   |
|    Math: Vec2/3/4, Mat3/4, Quat, Transform, BBox        |
+==========================================================+
|              THIRD PARTY (GLM, stb_image)                |
+==========================================================+
```

## Directory Layout

```
GameEngine/
+-- CMakeLists.txt          # Root build definition
+-- src/
|   +-- core/               # Types, Logger
|   +-- math/               # Vector, Matrix, Quaternion, Transform
|   +-- render/             # Renderer IF, OpenGL backend
|   +-- scene/              # Entity, Component, Scene, Camera
|   +-- resource/           # ResourceManager, ModelLoader
|   +-- engine/             # Engine, Window, Timer, Input
|   +-- demo/               # DemoApp, main.cpp (replaced by game/)
|   +-- game/               # Voxel game application
|       +-- Block.h/.cpp           Block types and properties
|       +-- Noise.h/.cpp           Simplex noise generator
|       +-- Chunk.h/.cpp           16x128x16 chunk + mesh gen
|       +-- World.h/.cpp           Chunk manager + terrain gen
|       +-- TerrainGenerator.h/.cpp Procedural terrain
|       +-- TreeGenerator.h/.cpp   Oak tree placement
|       +-- PlayerController.h/.cpp First-person controls
|       +-- BlockRaycast.h/.cpp    DDA voxel raycasting
|       +-- ParticleSystem.h/.cpp  Block break particles
|       +-- SoundManager.h/.cpp    Sound effects (Beep API)
|       +-- VoxelGame.h/.cpp       Main game orchestrator
|       +-- main.cpp               Game entry point
+-- shaders/               # GLSL shader source files
+-- third_party/           # Dependencies (GLM, stb_image)
+-- scripts/               # Build/dev scripts
+-- tests/                 # Unit tests
+-- docs/                  # Documentation
```

## Layer Dependencies

Each layer only depends on layers below it:

| Layer              | Depends On                         |
|--------------------|------------------------------------|
| Game (VoxelGame)   | Engine, Scene, Render, World, ...  |
| World/Chunk        | Engine, Core, Math, Render         |
| Demo               | Engine, Scene, Render              |
| Scene/ECS          | Core, Math, Render                 |
| Resource           | Core, Render                       |
| Render             | Core, Math                         |
| Engine             | Core, Math, Render, Scene, Resource|
| Core/Math          | GLM                                |

## Game Module (src/game/)

The game module is a Minecraft-like voxel world application:

### Key Systems

**World System**: Chunk-based infinite procedural world with 16x128x16 blocks per chunk.
Mesh generation uses face culling (only visible faces are rendered).

**Terrain Generation**: Multi-octave Simplex noise with biome system.
- Plains, Forest, Desert, Snowy, Ocean biomes
- Cave carving using 3D noise
- Tree placement in forests and plains

**Player**: First-person controller with WASD movement, mouse look, jumping.
Position tracking with simple ground collision.

**Block Interaction**: DDA (Digital Differential Analyzer) ray-walking algorithm
for efficient voxel picking. Left-click to break, right-click to place.

**Particles**: Per-cube particle system for block break effects with gravity,
damping, and alpha fading.

**Sound**: Simple Win32 Beep API for block break/place sounds.

### Planned Modules
- **Physics**: Rigid body dynamics, collision detection
- **Audio**: WAV playback via PlaySound
- **UI System**: Hotbar, crosshair, debug overlay
- **Day/Night Cycle**: Dynamic lighting
- **Inventory**: Block selection and crafting

## Render Backend Interface

The `Renderer` class is a pure virtual interface. To add a new backend:
1. Create your Renderer that inherits from `Renderer`
2. Implement all pure virtual methods
3. Add to the switch in Engine::Initialize()
4. No changes needed in Scene, Engine, or Game
