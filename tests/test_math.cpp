#include "Types.h"
#include "Vector.h"
#include "Matrix.h"
#include "Quaternion.h"
#include "Transform.h"
#include "Math.h"
#include <cassert>
#include <cmath>
#include <cstdio>

using namespace nebula;

static int g_testsPassed = 0;
static int g_testsFailed = 0;

#define TEST(name, expr) \
    do { \
        if (!(expr)) { \
            printf("  FAIL: %s\n", name); \
            g_testsFailed++; \
        } else { \
            printf("  PASS: %s\n", name); \
            g_testsPassed++; \
        } \
    } while(0)

int main() {
    printf("=== Nebula Engine Math Tests ===\n");
    
    // ?? Vector Tests ??
    {
        Vec3 v(1.0f, 2.0f, 3.0f);
        TEST("Vec3 construction", 
             fabs(v.x - 1.0f) < 0.001f && 
             fabs(v.y - 2.0f) < 0.001f && 
             fabs(v.z - 3.0f) < 0.001f);
        
        Vec3 sum = v + Vec3(1.0f, 1.0f, 1.0f);
        TEST("Vec3 addition", 
             fabs(sum.x - 2.0f) < 0.001f && 
             fabs(sum.y - 3.0f) < 0.001f && 
             fabs(sum.z - 4.0f) < 0.001f);
        
        f32 dotVal = glm::dot(v, Vec3(1.0f, 0.0f, 0.0f));
        TEST("Vec3 dot product", fabs(dotVal - 1.0f) < 0.001f);
        
        Vec3 crossVal = glm::cross(Vec3(1,0,0), Vec3(0,1,0));
        TEST("Vec3 cross product", 
             fabs(crossVal.x) < 0.001f && 
             fabs(crossVal.y) < 0.001f && 
             fabs(crossVal.z - 1.0f) < 0.001f);
    }
    
    // ?? Matrix Tests ??
    {
        Mat4 identity = Mat4(1.0f);
        TEST("Mat4 identity diagonal", 
             fabs(identity[0][0] - 1.0f) < 0.001f && 
             fabs(identity[1][1] - 1.0f) < 0.001f && 
             fabs(identity[2][2] - 1.0f) < 0.001f && 
             fabs(identity[3][3] - 1.0f) < 0.001f);
        
        Vec3 translation(5.0f, 0.0f, 0.0f);
        Mat4 trans = glm::translate(Mat4(1.0f), translation);
        Vec4 result = trans * Vec4(0.0f, 0.0f, 0.0f, 1.0f);
        TEST("Mat4 translation", fabs(result.x - 5.0f) < 0.001f);
        
        Mat4 inv = Inverse(trans);
        Vec4 invResult = inv * Vec4(5.0f, 0.0f, 0.0f, 1.0f);
        TEST("Mat4 inverse translation", fabs(invResult.x) < 0.001f);
    }
    
    // ?? Quaternion Tests ??
    {
        Quat identity = QuatIdentity();
        TEST("Quat identity", 
             fabs(identity.w - 1.0f) < 0.001f && 
             fabs(identity.x) < 0.001f && 
             fabs(identity.y) < 0.001f && 
             fabs(identity.z) < 0.001f);
        
        // 90 degree rotation around Y
        Quat rot = AngleAxis(90.0f, Vec3(0.0f, 1.0f, 0.0f));
        Vec3 forward = rot * Vec3(0.0f, 0.0f, -1.0f);
        TEST("Quat rotation 90Y", 
             fabs(forward.x - 1.0f) < 0.001f && 
             fabs(forward.y) < 0.001f && 
             fabs(forward.z) < 0.001f);
    }
    
    // ?? Transform Tests ??
    {
        Transform t;
        t.position = Vec3(1.0f, 2.0f, 3.0f);
        Mat4 m = t.ToMat4();
        Vec4 pos = m * Vec4(0.0f, 0.0f, 0.0f, 1.0f);
        TEST("Transform position", 
             fabs(pos.x - 1.0f) < 0.001f && 
             fabs(pos.y - 2.0f) < 0.001f && 
             fabs(pos.z - 3.0f) < 0.001f);
        
        // Test forward
        t.rotation = AngleAxis(90.0f, Vec3(0.0f, 1.0f, 0.0f));
        Vec3 fwd = t.Forward();
        TEST("Transform forward after rotation", 
             fabs(fwd.x - 1.0f) < 0.001f && 
             fabs(fwd.y) < 0.001f && 
             fabs(fwd.z) < 0.001f);
    }
    
    // ?? Math Utility Tests ??
    {
        TEST("Radians conversion", fabs(Radians(180.0f) - kPi) < 0.001f);
        TEST("Degrees conversion", fabs(Degrees(kPi) - 180.0f) < 0.001f);
        TEST("Clamp lower", fabs(Clamp(-1.0f, 0.0f, 1.0f)) < 0.001f);
        TEST("Clamp upper", fabs(Clamp(2.0f, 0.0f, 1.0f) - 1.0f) < 0.001f);
        TEST("Clamp middle", fabs(Clamp(0.5f, 0.0f, 1.0f) - 0.5f) < 0.001f);
        TEST("Lerp", fabs(Lerp(0.0f, 10.0f, 0.5f) - 5.0f) < 0.001f);
    }
    
    // ?? Camera / Matrix Tests ??
    {
        Mat4 proj = Perspective(60.0f, 16.0f/9.0f, 0.1f, 100.0f);
        // Perspective matrix should have non-zero elements
        TEST("Perspective matrix created", 
             fabs(proj[0][0]) > 0.0f && fabs(proj[1][1]) > 0.0f);
        
        Mat4 view = LookAt(Vec3(0.0f, 0.0f, 5.0f), Vec3(0.0f, 0.0f, 0.0f), Vec3(0.0f, 1.0f, 0.0f));
        // Camera at z=5 looking at origin
        Vec4 eyeInView = view * Vec4(0.0f, 0.0f, 5.0f, 1.0f);
        TEST("LookAt: eye at z=5 is origin in view space",
             fabs(eyeInView.x) < 0.001f && 
             fabs(eyeInView.y) < 0.001f && 
             fabs(eyeInView.z) < 0.001f);
    }
    
    printf("=== Results: %d passed, %d failed ===\n", g_testsPassed, g_testsFailed);
    return g_testsFailed > 0 ? 1 : 0;
}
