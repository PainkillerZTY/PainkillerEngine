# INBOX: Complete Multiplayer Network Sync

## Priority: MEDIUM
## Estimated Time: 6-8 hours

## Task
Implement full client-server multiplayer synchronization.

## Current State
- TCP server on port 25565 (src/game/Network.cpp)
- Accepts connections, tracks clients
- Basic Broadcast() function exists
- Client can connect but no sync happens

## What's Needed
1. Player position sync (server broadcasts to all clients)
2. Block change sync (when a block breaks/places, notify clients)
3. Chunk data sync (send chunk data to connecting clients)
4. Player join/leave notifications

## Protocol Design
```
Packet types:
0x01: PlayerPosition (x, y, z, yaw, pitch)
0x02: BlockChange (x, y, z, blockType)
0x03: ChunkData (chunkX, chunkZ, blockData[])
0x04: PlayerJoin (playerName)
0x05: PlayerLeave (playerId)
```

## Files to Modify
- `src/game/Network.h` - Add send/receive methods
- `src/game/Network.cpp` - Implement sync logic
- `src/game/Packet.h` - Add packet type definitions
- `src/game/Packet.cpp` - Add serialization for new types
- `src/game/VoxelGame.cpp` - Hook block changes to network

## Verification
- [ ] Two instances can connect and see each other
- [ ] Block changes sync between clients
- [ ] Newly connecting player gets world state
- [ ] Disconnects handled gracefully
