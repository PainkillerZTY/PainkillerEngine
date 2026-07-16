# Task 003: Fix Block Placement + Minor Bugs

## Metadata
- **Priority**: HIGH
- **Estimated Time**: 1-2 hours
- **Dependencies**: T-001 (greedy mesh fixed)

## Problem
Right-click to place blocks does not work. Hotbar shows blocks but placing
them in the world has no effect. Also, some minor rendering issues with flat colors.

## Files to Modify
- `src/game/VoxelGame.cpp` - HandleBlockInteraction() and GetSelectedBlock()
- `src/game/BlockRaycast.cpp` - Verify raycast returns correct results

## Investigation Steps
1. Check if `m_hotbarBlocks[]` is properly initialized in Initialize()
2. Check if `GetSelectedBlock()` returns the correct block type
3. Check if the raycast `m_raycastResult` has correct place position
4. Verify `m_placeCooldown` allows placement
5. Test with simple dirt block first

## Possible Fixes
- If `m_hotbarBlocks[]` was lost during git operations, re-add initialization
- If raycast returns air, check direction vector is normalized
- If cooldown is stuck, check m_placeCooldown decrement logic

## Verification
- [ ] Right-click places block at targeted face
- [ ] All 9 hotbar slots work
- [ ] Can break blocks with left-click
- [ ] Build: `cmake --build build --target painkiller`
