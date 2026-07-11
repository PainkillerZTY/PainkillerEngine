# Nebula Engine v0.1.0 — Minecraft-Like Voxel Game

A voxel-based sandbox game built from scratch with C++17, OpenGL 3.3 Core, and CMake.

## Features

### Current (MVP)
- **Procedural World**: Infinite terrain generation with Simplex noise
- **Biomes**: Plains, Forest, Desert, Snowy — with appropriate block types
- **Caves**: 3D noise-based cave carving
- **Trees**: Procedural oak tree generation in forests
- **First-Person Controls**: WASD + mouse look + jump
- **Block Interaction**: Left-click to break, right-click to place blocks
- **Particle Effects**: Block break particles with physics and fading
- **Sound Effects**: Block break/place sounds via Win32 Beep API
- **Distance Fog**: Smooth fog rendering for far-away chunks
- **Crosshair**: UI overlay crosshair

### Block Types
Grass, Dirt, Stone, Cobblestone, Oak Log, Leaves, Oak Planks, Sand, Water, Snow, Bedrock

## Building

### Prerequisites
- CMake 3.20+
- C++17 compiler (MSVC or MinGW GCC 6.3+)
- OpenGL 3.3 compatible GPU

### Quick Start
```bash
# Clone or navigate to project root
cd NebulaEngine

# Setup dependencies
powershell -ExecutionPolicy Bypass -File scripts/setup_deps.ps1

# Build
cmake -B build -G "MinGW Makefiles"
cmake --build build

# Run
./build/bin/nebula_game.exe
```

## Project Structure
See [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md) for details.

## Controls
| Key | Action |
|-----|--------|
| W/A/S/D | Move forward/left/backward/right |
| Mouse | Look around |
| Left Click | Break block |
| Right Click | Place block (Dirt) |
| Space | Jump |
| Escape | Quit |

## Roadmap
See [docs/ROADMAP.md](docs/ROADMAP.md) for planned features.

## License
MIT
