# Task 002: Multi-threaded Chunk Generation

## Assigned To: AI Employee #4 (World Generation Engineer)
## Status: PENDING
## Priority: HIGH

## Problem
Chunk generation runs on the main thread, causing frame drops when loading chunks.
Terrain generation (noise) + mesh generation takes ~10-20ms per chunk.

## Solution
Use enkiTS (already downloaded at `third_party/enkiTS.h`) to generate chunks 
in background threads.

## Files to Modify
- `src/game/World.cpp` - LoadChunk, ProcessChunkLoadQueue
- `src/game/World.h` - Add task scheduler member
- `src/game/CMakeLists.txt` - Add enkiTS.cpp

## Implementation Steps
1. Add enkiTS.cpp to CMakeLists.txt source list
2. Create enkiTS::TaskScheduler in World class
3. Split chunk loading into: 
   a) Generate terrain data (main thread, fast)
   b) Generate mesh (background thread via enkiTS)
   c) Upload mesh to GPU (main thread)
4. Use atomics or mutex for thread safety

## Reference
enkiTS API:
```cpp
#include "enkiTS.h"
enki::TaskScheduler ts;
ts.Initialize();
struct Task : enki::ITaskSet { void ExecuteRange(...) override {} };
Task task;
ts.AddTaskSetToPipe(&task);
ts.WaitforTask(&task);
```

## Verification
- [ ] Chunks generate without visual glitches
- [ ] No frame drops during chunk loading
- [ ] All 49 chunks load within 2 seconds
- [ ] No data races or crashes
