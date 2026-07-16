# Coding Standards

## C++ Standards
- C++17 (use std::optional, std::variant, structured bindings)
- Spaces not tabs, 4-space indent
- 100 character line limit
- CamelCase for classes: `VoxelGame`, `PlayerController`
- m_ prefix for member variables: `m_initialized`
- k prefix for constants: `kMaxChunks`

## Naming Conventions
| Element | Convention | Example |
|---------|-----------|---------|
| Classes | PascalCase | `BlockRaycast` |
| Functions | PascalCase | `GenerateMesh()` |
| Variables | camelCase | `deltaTime` |
| Members | m_camelCase | `m_world` |
| Constants | kPascalCase | `kBreakTime` |
| Enums | PascalCase | `BlockType::Grass` |

## Git Commit Format
`[module] Description of change (#issue)`

Modules: RENDER, GAMEPLAY, WORLD, NETWORK, CORE, DOCS, BUILD

## Code Review Checklist
- [ ] Compiles without warnings
- [ ] Follows naming conventions
- [ ] Comments explain WHY not WHAT
- [ ] No hardcoded magic numbers
- [ ] Error handling for all edge cases
- [ ] Performance: no O(n?) algorithms in hot path
