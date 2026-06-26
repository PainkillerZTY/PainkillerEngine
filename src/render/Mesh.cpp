#include "Mesh.h"
#include <cmath>

namespace nebula {

// ?? Cube ??
MeshData MeshData::CreateCube(f32 size) {
    MeshData data;
    f32 h = size * 0.5f;
    
    // Vertices: position (3) + normal (3) + texcoord (2) = 8 floats
    data.vertexStride = 8 * sizeof(f32);
    
    // Cube vertex data (24 vertices for proper normals on each face)
    f32 verts[] = {
        // Back face
        -h, -h, -h,  0.0f, 0.0f, -1.0f,  0.0f, 0.0f,
         h, -h, -h,  0.0f, 0.0f, -1.0f,  1.0f, 0.0f,
         h,  h, -h,  0.0f, 0.0f, -1.0f,  1.0f, 1.0f,
        -h,  h, -h,  0.0f, 0.0f, -1.0f,  0.0f, 1.0f,
        // Front face
        -h, -h,  h,  0.0f, 0.0f,  1.0f,  0.0f, 0.0f,
         h, -h,  h,  0.0f, 0.0f,  1.0f,  1.0f, 0.0f,
         h,  h,  h,  0.0f, 0.0f,  1.0f,  1.0f, 1.0f,
        -h,  h,  h,  0.0f, 0.0f,  1.0f,  0.0f, 1.0f,
        // Left face
        -h, -h, -h, -1.0f, 0.0f, 0.0f,  0.0f, 0.0f,
        -h, -h,  h, -1.0f, 0.0f, 0.0f,  1.0f, 0.0f,
        -h,  h,  h, -1.0f, 0.0f, 0.0f,  1.0f, 1.0f,
        -h,  h, -h, -1.0f, 0.0f, 0.0f,  0.0f, 1.0f,
        // Right face
         h, -h, -h,  1.0f, 0.0f, 0.0f,  0.0f, 0.0f,
         h, -h,  h,  1.0f, 0.0f, 0.0f,  1.0f, 0.0f,
         h,  h,  h,  1.0f, 0.0f, 0.0f,  1.0f, 1.0f,
         h,  h, -h,  1.0f, 0.0f, 0.0f,  0.0f, 1.0f,
        // Bottom face
        -h, -h, -h,  0.0f, -1.0f, 0.0f,  0.0f, 0.0f,
         h, -h, -h,  0.0f, -1.0f, 0.0f,  1.0f, 0.0f,
         h, -h,  h,  0.0f, -1.0f, 0.0f,  1.0f, 1.0f,
        -h, -h,  h,  0.0f, -1.0f, 0.0f,  0.0f, 1.0f,
        // Top face
        -h,  h, -h,  0.0f, 1.0f, 0.0f,  0.0f, 0.0f,
         h,  h, -h,  0.0f, 1.0f, 0.0f,  1.0f, 0.0f,
         h,  h,  h,  0.0f, 1.0f, 0.0f,  1.0f, 1.0f,
        -h,  h,  h,  0.0f, 1.0f, 0.0f,  0.0f, 1.0f,
    };
    
    data.vertices.assign(verts, verts + 192);
    data.vertexCount = 24;
    
    // Indices (36 indices, 12 triangles)
    u32 idx[] = {
        0,1,2, 0,2,3,      // back
        4,6,5, 4,7,6,      // front
        8,9,10, 8,10,11,   // left
        12,14,13, 12,15,14, // right
        16,17,18, 16,18,19, // bottom
        20,22,21, 20,23,22, // top
    };
    
    data.indices.assign(idx, idx + 36);
    data.indexCount = 36;
    
    return data;
}

// ?? Sphere ??
MeshData MeshData::CreateSphere(f32 radius, u32 segments, u32 rings) {
    MeshData data;
    data.vertexStride = 8 * sizeof(f32); // pos(3) + normal(3) + texcoord(2)

    for (u32 ring = 0; ring <= rings; ++ring) {
        f32 theta = (f32)ring * kPi / (f32)rings;
        f32 sinTheta = sinf(theta);
        f32 cosTheta = cosf(theta);

        for (u32 seg = 0; seg <= segments; ++seg) {
            f32 phi = (f32)seg * kTwoPi / (f32)segments;
            f32 sinPhi = sinf(phi);
            f32 cosPhi = cosf(phi);

            Vec3 normal(sinTheta * cosPhi, cosTheta, sinTheta * sinPhi);
            Vec3 pos = normal * radius;
            f32 u = (f32)seg / (f32)segments;
            f32 v = (f32)ring / (f32)rings;

            data.vertices.push_back(pos.x); data.vertices.push_back(pos.y); data.vertices.push_back(pos.z);
            data.vertices.push_back(normal.x); data.vertices.push_back(normal.y); data.vertices.push_back(normal.z);
            data.vertices.push_back(u); data.vertices.push_back(v);
        }
    }

    for (u32 ring = 0; ring < rings; ++ring) {
        for (u32 seg = 0; seg < segments; ++seg) {
            u32 first = ring * (segments + 1) + seg;
            u32 second = first + segments + 1;

            data.indices.push_back(first);
            data.indices.push_back(second);
            data.indices.push_back(first + 1);

            data.indices.push_back(first + 1);
            data.indices.push_back(second);
            data.indices.push_back(second + 1);
        }
    }

    data.vertexCount = (u32)data.vertices.size() / 8;
    data.indexCount = (u32)data.indices.size();

    return data;
}

// ?? Plane ??
MeshData MeshData::CreatePlane(f32 width, f32 depth) {
    MeshData data;
    data.vertexStride = 8 * sizeof(f32);
    
    f32 hw = width * 0.5f;
    f32 hd = depth * 0.5f;

    f32 verts[] = {
        -hw, 0.0f, -hd,  0.0f, 1.0f, 0.0f,  0.0f, 0.0f,
         hw, 0.0f, -hd,  0.0f, 1.0f, 0.0f,  1.0f, 0.0f,
         hw, 0.0f,  hd,  0.0f, 1.0f, 0.0f,  1.0f, 1.0f,
        -hw, 0.0f,  hd,  0.0f, 1.0f, 0.0f,  0.0f, 1.0f,
    };
    
    data.vertices.assign(verts, verts + 32);
    data.vertexCount = 4;
    
    u32 idx[] = { 0, 1, 2, 0, 2, 3 };
    data.indices.assign(idx, idx + 6);
    data.indexCount = 6;
    
    return data;
}

// ?? Triangle ??
MeshData MeshData::CreateTriangle() {
    MeshData data;
    data.vertexStride = 8 * sizeof(f32);
    
    f32 verts[] = {
        0.0f,  0.5f, 0.0f,  0.0f, 0.0f, 1.0f,  0.5f, 1.0f,
       -0.5f, -0.5f, 0.0f,  0.0f, 0.0f, 1.0f,  0.0f, 0.0f,
        0.5f, -0.5f, 0.0f,  0.0f, 0.0f, 1.0f,  1.0f, 0.0f,
    };
    
    data.vertices.assign(verts, verts + 24);
    data.vertexCount = 3;
    
    return data;
}

} // namespace nebula
