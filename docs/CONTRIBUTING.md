# Contributing Guide

## GitHub Workflow
1. Pick an issue from GitHub Issues
2. Create branch: `feature/description` or `fix/description`
3. Make changes following CODING_STANDARDS.md
4. Test: `cmake --build build --target painkiller`
5. Commit: `[MODULE] Description (#issue)`
6. Push and create Pull Request to `main`
7. Wait for Codex review

## Directory Structure
```
src/
  core/     - Types, Logger (engine-agnostic)
  math/     - GLM wrappers
  render/   - Renderer interface + OpenGL impl
  scene/    - Camera, Entity, Scene graph
  resource/ - Texture loading, ResourceManager
  engine/   - Window, Input, Timer, Game loop
  game/     - VoxelGame, World, Chunk, Blocks, Mobs
third_party/ - External libraries
shaders/    - GLSL shader files (if extracted)
docs/       - Documentation
assets/     - Textures, sounds, models
```

## Key Files
| File | Purpose | Lines |
|------|---------|-------|
| `src/game/VoxelGame.h` | Main game class header | ~250 |
| `src/game/VoxelGame.cpp` | Main game implementation | ~1800 |
| `src/game/Chunk.cpp` | Block storage + mesh generation | ~300 |
| `src/game/World.cpp` | Chunk management + rendering | ~350 |
| `src/render/OpenGLRenderer.cpp` | OpenGL backend | ~400 |
| `src/engine/Engine.cpp` | Game loop + ImGui | ~200 |
