# Nebula Engine — Minecraft-Like Game Roadmap

## Project Goal
Turn NebulaEngine v0.1.0 into a playable Minecraft-like sandbox game with:

- Procedural voxel world generation
- First-person exploration, block breaking/placing
- Trees, sound, particles, and basic UI

## Phase 1: Project Initialization
- [x] Read and understand existing NebulaEngine codebase
- [x] Create ROADMAP.md
- [x] Update ARCHITECTURE.md with game module docs
- [x] Initialize git version control

## Phase 2: Voxel Engine Core
- [ ] Block.h — Block types, properties (solid/transparent/color)
- [ ] Chunk.h/cpp — 16x128x16 chunk data + mesh generation (face culling)
- [ ] World.h/cpp — Chunk container, dirty flag management, async mesh loading
- [ ] Procedural texture atlas generation for blocks

## Phase 3: Terrain Generation
- [ ] Noise.h/cpp — Simplex noise for height/temperature/humidity
- [ ] TerrainGenerator.h/cpp — Biome-based terrain: grass, dirt, stone, sand, water, snow
- [ ] Chunk loading/unloading relative to player position

## Phase 4: Game Systems
- [ ] First-person player controller (WASD + mouse look + jump)
- [ ] Collision detection (AABB against blocks)
- [ ] Block raycasting (pick block from camera)
- [ ] Block breaking (left-click with animation timer)
- [ ] Block placing (right-click)

## Phase 5: Trees & Vegetation
- [ ] TreeGenerator.h/cpp — Oak tree generation (trunk + canopy)
- [ ] Tree placement during terrain generation
- [ ] Leaf transparency rendering
- [ ] Tree chopping (block break behavior)

## Phase 6: Visual Effects & Audio
- [ ] ParticleSystem.h/cpp — Block break particle effects (cube fragments)
- [ ] SoundManager.h/cpp — WAV playback via Win32 PlaySound API
- [ ] Block break/place sound effects (generated or loaded)
- [ ] Footstep sounds (optional)

## Phase 7: UI
- [ ] Crosshair rendering
- [ ] Hotbar / block selection UI
- [ ] Debug info overlay (FPS, position, block targeted)

## Phase 8: Polish & Release
- [ ] Basic lighting (ambient occlusion, smooth lighting)
- [ ] Water rendering (transparent animated surface)
- [ ] Crafting / inventory (stretch goal)
- [ ] Day/night cycle (stretch goal)
- [ ] Build verification & testing
- [ ] Final git tagging

---
*Last updated: 2026-07-11*
