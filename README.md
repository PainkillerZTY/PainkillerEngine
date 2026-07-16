# Painkiller Engine — Minecraft-Like Voxel Game

## Overview
A voxel sandbox game built from scratch with C++17 + OpenGL 3.3 Core.
Procedural terrain, first-person controls, block interaction, water physics.

## Quick Start
`ash
powershell -File scripts/setup_deps.ps1
cmake -B build -G Ninja
cmake --build build
./build/bin/painkiller.exe
`

## Controls
| Key | Action | Key | Action |
|-----|--------|-----|--------|
| WASD | Move | Space | Jump |
| Mouse | Look | Shift | Sprint |
| LMB | Break block | Ctrl | Sneak |
| RMB | Place block | F | Toggle flight |
| 1-9/Scroll | Hotbar | Esc | Quit |

## AI Team Structure
See [docs/TEAM.md](docs/TEAM.md) for team roles and communication.
See [docs/HANDOFF.md](docs/HANDOFF.md) for project handoff to AI developers.

## Documentation
| Document | Description |
|----------|-------------|
| [ARCHITECTURE.md](ARCHITECTURE.md) | System architecture |
| [docs/TEAM.md](docs/TEAM.md) | Team structure |
| [docs/HANDOFF.md](docs/HANDOFF.md) | AI developer handoff |
| [docs/CODING_STANDARDS.md](docs/CODING_STANDARDS.md) | C++ coding standards |
| [docs/CONTRIBUTING.md](docs/CONTRIBUTING.md) | Git workflow |
| [docs/ROADMAP.md](docs/ROADMAP.md) | Development roadmap |

## Build
```
cmake -B build -G Ninja
cmake --build build --target painkiller
./build/bin/painkiller.exe
```

## Features
- **World**: Infinite terrain, 4 biomes, caves, trees
- **Blocks**: 10 types in 64×48 texture atlas
- **Water**: Real-time flow (down/horizontal spread)
- **Physics**: AABB collision, gravity, flight
- **Rendering**: Frustum culling, fog, procedural sky
- **Audio**: Win32 Beep API block sounds
- **UI**: Crosshair, 9-slot hotbar, held block

## Project Structure
`
PainkillerEngine/
├── src/
│   ├── core/        # Types, logging
│   ├── math/        # Vectors, matrices (GLM)
│   ├── render/      # Abstract GPU API + OpenGL 3.3
│   ├── scene/       # Entity/component system, camera
│   ├── resource/    # Texture/model loading (stb_image)
│   ├── engine/      # Window, input, main loop
│   └── game/        # Voxel world, gameplay
├── assets/          # Textures, skybox images
├── shaders/         # GLSL shaders (not used; embedded)
├── scripts/         # Build helpers
├── docs/            # Architecture, roadmap
├── third_party/     # GLM, stb_image
├── CMakeLists.txt
└── README.md
`

## Dependencies
- **GLM** — Math library (vectors, matrices, noise)
- **stb_image** — Image loading
- **Ninja** — Build system

## License MIT
