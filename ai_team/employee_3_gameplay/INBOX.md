# INBOX: Fix Block Placement + Crafting System

## Priority: HIGH
## Estimated Time: 2-3 hours

## Task
Fix block placement (right-click doesn't place blocks) and improve crafting.

## Investigation
1. `src/game/VoxelGame.cpp` - HandleBlockInteraction()
2. `src/game/VoxelGame.cpp` - GetSelectedBlock()
3. `src/game/BlockRaycast.cpp` - DDA raycast

## Known Issues
- Right-click doesn't place blocks in the world
- Hotbar blocks shown but can't be placed
- Crafting table GUI works but recipes incomplete

## Files to Modify
- `src/game/VoxelGame.cpp` - Fix block placement logic
- `src/game/BlockRaycast.cpp` - Verify raycast results
- `src/game/VoxelGame.h` - If new member variables needed

## Debug Steps
1. Check m_inventory is initialized
2. Check m_placeCooldown decrements properly
3. Check raycast direction is normalized
4. Check GetSelectedBlock() returns non-Air block type

## Expected Result
- Right-click places the selected block
- All 9 hotbar slots work
- Can break blocks with left-click (hold)
