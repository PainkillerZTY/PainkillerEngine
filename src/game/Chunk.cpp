#include "Chunk.h"
#include "Logger.h"
#include <cstring>

namespace painkiller {

// ============================================================
// Construction
// ============================================================
Chunk::Chunk(i32 chunkX, i32 chunkZ)
    : m_chunkX(chunkX), m_chunkZ(chunkZ) {
    m_blocks.fill(BlockType::Air);
}

// ============================================================
// Block access
// ============================================================
bool Chunk::IsLocalCoordValid(i32 x, i32 y, i32 z) const {
    return x >= 0 && x < CHUNK_SIZE_X &&
           y >= 0 && y < CHUNK_SIZE_Y &&
           z >= 0 && z < CHUNK_SIZE_Z;
}

BlockType Chunk::GetBlock(i32 x, i32 y, i32 z) const {
    if (!IsLocalCoordValid(x, y, z)) return BlockType::Air;
    return m_blocks[BlockIndex(x, y, z)];
}

void Chunk::SetBlock(i32 x, i32 y, i32 z, BlockType type) {
    if (!IsLocalCoordValid(x, y, z)) return;
    m_blocks[BlockIndex(x, y, z)] = type;
    m_meshDirty = true;
}

// ============================================================
// Face vertex offsets from block origin
// Each face returns 4 corner positions relative to (0,0,0)
// ============================================================
static const Vec3 kFaceVerts[6][4] = {
    // +X (Right)
    { Vec3(1,0,0), Vec3(1,1,0), Vec3(1,1,1), Vec3(1,0,1) },
    // -X (Left)
    { Vec3(0,0,1), Vec3(0,1,1), Vec3(0,1,0), Vec3(0,0,0) },
    // +Y (Top) - CCW when viewed from +Y
    { Vec3(0,1,0), Vec3(0,1,1), Vec3(1,1,1), Vec3(1,1,0) },
    // -Y (Bottom) - CCW when viewed from -Y
    { Vec3(0,0,0), Vec3(1,0,0), Vec3(1,0,1), Vec3(0,0,1) },
    // +Z (Front)
    { Vec3(1,0,1), Vec3(1,1,1), Vec3(0,1,1), Vec3(0,0,1) },
    // -Z (Back)
    { Vec3(0,0,0), Vec3(0,1,0), Vec3(1,1,0), Vec3(1,0,0) },
};

static const Vec3 kFaceNormals[6] = {
    Vec3( 1, 0, 0),  // +X Right
    Vec3(-1, 0, 0),  // -X Left
    Vec3( 0, 1, 0),  // +Y Top
    Vec3( 0,-1, 0),  // -Y Bottom
    Vec3( 0, 0, 1),  // +Z Front
    Vec3( 0, 0,-1),  // -Z Back
};

static const i32 kFaceChecks[6][3] = {
    { 1, 0, 0},  // +X
    {-1, 0, 0},  // -X
    { 0, 1, 0},  // +Y
    { 0,-1, 0},  // -Y
    { 0, 0, 1},  // +Z
    { 0, 0,-1},  // -Z
};

static bool ShouldRenderFace(BlockType blockType, BlockType neighborType) {
    if (blockType == BlockType::Air) return false;
    // Render face if neighbor is air, water, or leaves (transparent)
    return neighborType == BlockType::Air ||
           neighborType == BlockType::Water ||
           neighborType == BlockType::Leaves;
}

// ============================================================
// Mesh Generation
// ============================================================
void Chunk::GenerateMesh(i32 chunkX, i32 chunkZ) {
    m_vertices.clear();
    m_indices.clear();
    m_vertexCount = 0;
    m_indexCount = 0;
    m_hasMesh = false;

    if (!m_meshDirty) return;

    m_chunkX = chunkX;
    m_chunkZ = chunkZ;

    // World-space origin of this chunk
    f32 baseX = (f32)(chunkX * CHUNK_SIZE_X);
    f32 baseZ = (f32)(chunkZ * CHUNK_SIZE_Z);

    for (i32 y = 0; y < CHUNK_SIZE_Y; ++y) {
        for (i32 z = 0; z < CHUNK_SIZE_Z; ++z) {
            for (i32 x = 0; x < CHUNK_SIZE_X; ++x) {
                BlockType block = GetBlock(x, y, z);
                if (block == BlockType::Air) continue;

                // Check each face
                for (i32 face = 0; face < 6; ++face) {
                    i32 nx = x + kFaceChecks[face][0];
                    i32 ny = y + kFaceChecks[face][1];
                    i32 nz = z + kFaceChecks[face][2];

                    // Check neighbor - if out of chunk bounds, treat as visible
                    // (World will handle edge cases)
                    BlockType neighbor = BlockType::Air;
                    if (IsLocalCoordValid(nx, ny, nz)) {
                        neighbor = GetBlock(nx, ny, nz);
                    }

                    if (!ShouldRenderFace(block, neighbor)) continue;

                    // Add face quad
                    Vec3 worldOffset(baseX + (f32)x, (f32)y, baseZ + (f32)z);
                    Vec3 v0 = kFaceVerts[face][0] + worldOffset;
                    Vec3 v1 = kFaceVerts[face][1] + worldOffset;
                    Vec3 v2 = kFaceVerts[face][2] + worldOffset;
                    Vec3 v3 = kFaceVerts[face][3] + worldOffset;

                    AddFace(v0, v1, v2, v3, kFaceNormals[face], block, (BlockFace)face);
                }
            }
        }
    }

    m_meshDirty = false;
    m_hasMesh = m_indexCount > 0;
}

// ============================================================
// Add a face quad to the mesh (2 triangles, 4 vertices)
// Vertex format: pos(3) + normal(3) + texcoord(2) + uv(2) = 10 floats
// texcoord encodes (blockType/255, faceIndex/6)
// ============================================================

// LOD
void Chunk::GenerateLODMesh(i32 cx, i32 cz) {
    m_lodVertices.clear(); m_lodIndices.clear();
    m_lodVertexCount=0; m_lodIndexCount=0; m_hasLODMesh=false;
    f32 bx=(f32)(cx*16), bz=(f32)(cz*16);
    for(i32 y=0;y<128;y+=2) for(i32 z=0;z<16;z+=2) for(i32 x=0;x<16;x+=2){
        BlockType mt=BlockType::Air; bool fnd=false;
        for(i32 dy=0;dy<2&&!fnd;dy++) for(i32 dz=0;dz<2&&!fnd;dz++) for(i32 dx=0;dx<2&&!fnd;dx++){
            BlockType b=GetBlock(x+dx,y+dy,z+dz);
            if(b!=BlockType::Air){mt=b;fnd=true;}}
        if(!fnd)continue;
        Vec3 wo(bx+(f32)x,(f32)y,bz+(f32)z);
        for(i32 f=0;f<6;f++){
            i32 nx=x+(f==0?2:f==1?-2:0),ny=y+(f==2?2:f==3?-2:0),nz=z+(f==4?2:f==5?-2:0);
            BlockType nb=BlockType::Air;
            if(IsLocalCoordValid(nx,ny,nz)){
                for(i32 dy=0;dy<2&&nb==BlockType::Air;dy++) for(i32 dz=0;dz<2&&nb==BlockType::Air;dz++) for(i32 dx=0;dx<2&&nb==BlockType::Air;dx++)
                    nb=GetBlock(nx+dx,ny+dy,nz+dz);}
            if(nb!=BlockType::Air)continue;
            f32 tu=(f32)(u8)mt/255.0f, tv=(f32)f/6.0f;
            Vec3 v0=kFaceVerts[f][0]*2.0f+wo; Vec3 v1=kFaceVerts[f][1]*2.0f+wo;
            Vec3 v2=kFaceVerts[f][2]*2.0f+wo; Vec3 v3=kFaceVerts[f][3]*2.0f+wo;
            Vec3 vs[4]={v0,v1,v2,v3};
            for(i32 i=0;i<4;i++){
                m_lodVertices.push_back(vs[i].x);m_lodVertices.push_back(vs[i].y);m_lodVertices.push_back(vs[i].z);
                m_lodVertices.push_back(kFaceNormals[f].x);m_lodVertices.push_back(kFaceNormals[f].y);m_lodVertices.push_back(kFaceNormals[f].z);
                m_lodVertices.push_back(tu);m_lodVertices.push_back(tv);}
            u32 idx=m_lodVertexCount;
            m_lodIndices.push_back(idx);m_lodIndices.push_back(idx+1);m_lodIndices.push_back(idx+2);
            m_lodIndices.push_back(idx);m_lodIndices.push_back(idx+2);m_lodIndices.push_back(idx+3);
            m_lodVertexCount+=4;m_lodIndexCount+=6;}}
    m_hasLODMesh=m_lodIndexCount>0;}
void Chunk::AddFace(const Vec3& v0, const Vec3& v1, const Vec3& v2, const Vec3& v3,
                    const Vec3& normal, BlockType blockType, BlockFace face) {
    // Encode block type and face into texcoord
    f32 tu = (f32)(u8)blockType / 255.0f;
    f32 tv = (f32)(u8)face / 6.0f;

    Vec3 verts[4] = { v0, v1, v2, v3 };

    for (i32 i = 0; i < 4; ++i) {
        m_vertices.push_back(verts[i].x);
        m_vertices.push_back(verts[i].y);
        m_vertices.push_back(verts[i].z);
        m_vertices.push_back(normal.x);
        m_vertices.push_back(normal.y);
        m_vertices.push_back(normal.z);
        m_vertices.push_back(tu);
        m_vertices.push_back(tv);
    }

    u32 idx = m_vertexCount;
    m_indices.push_back(idx);
    m_indices.push_back(idx + 1);
    m_indices.push_back(idx + 2);
    m_indices.push_back(idx);
    m_indices.push_back(idx + 2);
    m_indices.push_back(idx + 3);

    m_vertexCount += 4;
    m_indexCount += 6;
}

} // namespace painkiller
