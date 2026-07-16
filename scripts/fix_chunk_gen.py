with open("src/game/Chunk.cpp","r",encoding="utf-8") as f: c = f.read()
idx = c.find("void Chunk::GenerateMesh")
nxt = c.find("void Chunk::GenerateLODMesh", idx)
old = c[idx:nxt]
new = """void Chunk::GenerateMesh(i32 chunkX, i32 chunkZ) {
    m_vertices.clear(); m_indices.clear();
    m_vertexCount = 0; m_indexCount = 0; m_hasMesh = false;
    if (!m_meshDirty) return;
    m_chunkX = chunkX; m_chunkZ = chunkZ;
    f32 baseX = (f32)(chunkX * CHUNK_SIZE_X);
    f32 baseZ = (f32)(chunkZ * CHUNK_SIZE_Z);
    for (i32 y = 0; y < CHUNK_SIZE_Y; ++y) {
        for (i32 z = 0; z < CHUNK_SIZE_Z; ++z) {
            for (i32 x = 0; x < CHUNK_SIZE_X; ++x) {
                BlockType block = GetBlock(x, y, z);
                if (block == BlockType::Air) continue;
                for (i32 face = 0; face < 6; ++face) {
                    i32 nx = x + kFaceChecks[face][0];
                    i32 ny = y + kFaceChecks[face][1];
                    i32 nz = z + kFaceChecks[face][2];
                    BlockType neighbor = BlockType::Air;
                    if (IsLocalCoordValid(nx, ny, nz)) neighbor = GetBlock(nx, ny, nz);
                    if (!ShouldRenderFace(block, neighbor)) continue;
                    Vec3 wo(baseX + (f32)x, (f32)y, baseZ + (f32)z);
                    Vec3 v0 = kFaceVerts[face][0] + wo;
                    Vec3 v1 = kFaceVerts[face][1] + wo;
                    Vec3 v2 = kFaceVerts[face][2] + wo;
                    Vec3 v3 = kFaceVerts[face][3] + wo;
                    AddFace(v0, v1, v2, v3, kFaceNormals[face], block, (BlockFace)face);
                }
            }
        }
    }
    m_meshDirty = false; m_hasMesh = m_indexCount > 0;
}
"""
c = c.replace(old, new)
with open("src/game/Chunk.cpp","w",encoding="utf-8") as f: f.write(c)
print("GenerateMesh replaced with per-face generation")