# Task 001: Fix Greedy Meshing Rendering Errors

## Assigned To: AI Employee #2 (Rendering Engineer)
## Status: PENDING
## Priority: CRITICAL

## Problem
The greedy meshing in `src/game/Chunk.cpp` produces rendering artifacts. 
The mask vector `std::vector<i8> mv(mh * mw)` is correct (no overflow),
but the vertex winding for some face directions may be wrong.

## Files to Modify
- `src/game/Chunk.cpp` - GenerateMesh() function
- `src/game/Chunk.h` - If AddFace needs changes

## Approach
1. Read the current GenerateMesh function
2. Verify vertex winding for all 6 face directions against kFaceVerts[]
3. If needed, add debug visualization or revert to per-face generation
4. Test: `cmake --build build --target painkiller && build/bin/painkiller.exe`

## Expected Result
All blocks render correctly with proper face orientation.
No visual artifacts on any block type.

## Verification
- [ ] Grass blocks: green top, green/brown sides, brown bottom
- [ ] All 24 block types render with correct colors
- [ ] No texture stretching or wrong face orientation
- [ ] Build compiles with no errors

## Fallback Plan
If greedy meshing cannot be fixed within 30 minutes:
1. Comment out the greedy meshing code
2. Restore the per-face generation (original loop)
3. Keep the pre-allocation (m_vertices.reserve)
4. Commit and move on
