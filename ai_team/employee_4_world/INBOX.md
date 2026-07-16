# INBOX: Multi-threaded Chunk Generation

## Priority: HIGH
## Estimated Time: 4-6 hours
## Dependencies: enkiTS (third_party/enkiTS.h - already downloaded)

## Task
Move chunk mesh generation to background threads using enkiTS.

## Problem
Terrain generation + mesh building runs on main thread, causing frame drops.
Each chunk takes ~10ms to generate, with 25 chunks that's 250ms of stutter.

## Solution
Split chunk loading into two phases:
1. Terrain DATA generation (main thread, fast - just fills block arrays)
2. Mesh VERTEX generation (background thread via enkiTS)
3. GPU upload (main thread - just creates GL buffers)

## Files to Modify
- `src/game/World.cpp` - LoadChunk, ProcessChunkLoadQueue, UploadChunkMesh
- `src/game/World.h` - Add enkiTS::TaskScheduler member
- `src/game/Chunk.h` - Maybe add thread-safe mesh data
- `src/game/CMakeLists.txt` - Add enkiTS.cpp

## enkiTS API Reference
```cpp
#include "enkiTS.h"
enki::TaskScheduler ts;
ts.Initialize();

struct GenTask : enki::ITaskSet {
    Chunk* chunk;
    void ExecuteRange(enki::TaskSetPartition range, uint32_t threadnum) override {
        chunk->GenerateMesh(chunk->GetChunkX(), chunk->GetChunkZ());
    }
};

GenTask task;
task.chunk = chunkPtr;
ts.AddTaskSetToPipe(&task);
// ... later:
ts.WaitforTask(&task);
```

## Verification
- [ ] Chunks generate without visual glitches
- [ ] No frame drops during chunk loading
- [ ] Thread count = CPU core count
- [ ] No data races or crashes
- [ ] Build compiles with -lpthread (MinGW needs this)
