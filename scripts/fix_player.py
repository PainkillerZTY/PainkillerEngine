import re
# Read header
h=open("G:/AI_projects/GameEngine/src/game/VoxelGame.h","r",encoding="utf-8").read()
# Add RenderPlayerModel declaration
h=h.replace("void RenderBlockHighlight(Renderer* renderer, Camera* camera);",
         "void RenderBlockHighlight(Renderer* renderer, Camera* camera);\n    void RenderPlayerModel(Renderer* renderer, Camera* camera, f32 deltaTime);",1)
# Add pipeline members
h=h.replace("MeshHandle m_wireframeCubeMesh = kInvalidHandle;",
         "MeshHandle m_wireframeCubeMesh = kInvalidHandle;\n    ShaderHandle m_playerVS = kInvalidHandle;\n    ShaderHandle m_playerFS = kInvalidHandle;\n    PipelineHandle m_playerPipeline = kInvalidHandle;\n    MeshHandle m_playerCubeMesh = kInvalidHandle;",1)
open("G:/AI_projects/GameEngine/src/game/VoxelGame.h","w",encoding="utf-8").write(h)
print("1. Header updated")

# Read cpp
c=open("G:/AI_projects/GameEngine/src/game/VoxelGame.cpp","r",encoding="utf-8").read()

# Add player pipeline after wireframe pipeline
old_pipe="        m_wireframePipeline=renderer->CreatePipelineState(wp);\n        }\n    }\n    \n    m_pipelineCreated = true;"
new_pipe="""        m_wireframePipeline=renderer->CreatePipelineState(wp);
        }
    }
    // Player model pipeline (simple colored 3D)
    {
        const char* pVS = R"(#version 330 core
            layout(location=0) in vec3 a_Position;
            uniform mat4 u_View; uniform mat4 u_Projection; uniform mat4 u_Model;
            void main() { gl_Position = u_Projection * u_View * u_Model * vec4(a_Position, 1.0); }
        )";
        const char* pFS = R"(#version 330 core
            uniform vec4 u_Color; out vec4 FragColor;
            void main() { FragColor = u_Color; }
        )";
        ShaderDesc pvs; pvs.stage=ShaderStage::Vertex; pvs.source=pVS;
        ShaderDesc pfs; pfs.stage=ShaderStage::Fragment; pfs.source=pFS;
        m_playerVS=renderer->CreateShader(pvs);
        m_playerFS=renderer->CreateShader(pfs);
        if(m_playerVS!=kInvalidHandle && m_playerFS!=kInvalidHandle){
            PipelineStateDesc pp;
            pp.vertexShader=&pvs; pp.fragmentShader=&pfs;
            pp.topology=PrimitiveTopology::TriangleList;
            pp.cullMode=CullMode::Back;
            pp.depthTestEnabled=true; pp.depthWriteEnabled=true;
            pp.blendMode=BlendMode::Opaque;
            pp.vertexLayout={{"POSITION",Format::R32G32B32_FLOAT,0,0}};
            m_playerPipeline=renderer->CreatePipelineState(pp);
        }
    }
    
    m_pipelineCreated = true;"""
c=c.replace(old_pipe,new_pipe,1)
print("2. Player pipeline added")

# Add cube mesh after wireframe mesh creation
old_mesh="        m_wireframeCubeMesh = renderer->CreateMesh(wf);\n    }"
new_mesh="""        m_wireframeCubeMesh = renderer->CreateMesh(wf);
    }
    // 10. Player cube mesh (unit cube, position-only stride=12)
    {
        MeshData pc;
        pc.vertexStride = 3*sizeof(f32);
        f32 v[] = {
            -0.5f,-0.5f,-0.5f, 0.5f,-0.5f,-0.5f, 0.5f, 0.5f,-0.5f, -0.5f, 0.5f,-0.5f,
            -0.5f,-0.5f, 0.5f, 0.5f,-0.5f, 0.5f, 0.5f, 0.5f, 0.5f, -0.5f, 0.5f, 0.5f
        };
        pc.vertices.assign(v, v+24);
        pc.vertexCount = 8;
        u32 idx[] = {0,1,2,0,2,3, 4,5,6,4,6,7, 0,4,7,0,7,3, 1,5,6,1,6,2, 3,7,6,3,6,2, 0,1,5,0,5,4};
        pc.indices.assign(idx, idx+36);
        pc.indexCount = 36;
        m_playerCubeMesh = renderer->CreateMesh(pc);
    }"""
c=c.replace(old_mesh,new_mesh,1)
print("3. Player cube mesh added")

# Add RenderPlayerModel call in Render
old_render="    RenderBlockHighlight(renderer, camera);\n    \n    // Always render UI"
new_render="    RenderBlockHighlight(renderer, camera);\n    RenderPlayerModel(renderer, camera, deltaTime);\n    \n    // Always render UI"
c=c.replace(old_render,new_render,1)
print("4. RenderPlayerModel call added")

# Implement RenderPlayerModel function
old_func="void VoxelGame::RenderBlockHighlight(Renderer* renderer, Camera* camera) {"
new_func="""struct BodyPart { float x,y,z, sx,sy,sz; float r,g,b; };
void VoxelGame::RenderPlayerModel(Renderer* renderer, Camera* camera, f32 deltaTime) {
    if(m_playerPipeline==kInvalidHandle||m_playerCubeMesh==kInvalidHandle)return;
    static float walkTime=0; walkTime+=deltaTime;
    float limbSin=sinf(walkTime*8.0f)*0.4f;
    // Body parts: head, body, leftArm, rightArm, leftLeg, rightLeg
    BodyPart parts[6]={
        {0,1.2f,0, 0.5f,0.5f,0.5f, 0.6f,0.4f,0.2f},  // head - brown
        {0,0.5f,0, 0.6f,0.7f,0.35f, 0.2f,0.4f,0.8f}, // body - blue
        {-0.5f,0.6f,0, 0.2f,0.7f,0.2f, 0.8f,0.3f,0.2f}, // leftArm - red
        {0.5f,0.6f,0, 0.2f,0.7f,0.2f, 0.8f,0.3f,0.2f},  // rightArm - red
        {-0.2f,0.1f,0, 0.2f,0.5f,0.2f, 0.3f,0.2f,0.1f}, // leftLeg - dark
        {0.2f,0.1f,0, 0.2f,0.5f,0.2f, 0.3f,0.2f,0.1f}   // rightLeg - dark
    };
    // Animation offsets for arms/legs
    parts[2].x=-0.5f+limbSin*0.2f; parts[2].z=limbSin*0.3f;
    parts[3].x=0.5f-limbSin*0.2f; parts[3].z=-limbSin*0.3f;
    parts[4].z=-limbSin*0.3f; parts[5].z=limbSin*0.3f;
    
    Vec3 pos=camera->GetPosition();
    Vec3 fwd=camera->GetForward();
    Vec3 right=camera->GetRight();
    Vec3 playerPos=pos-fwd*3.0f+Vec3(0,-0.5f,0);
    
    renderer->BindPipelineState(m_playerPipeline);
    renderer->SetUniformMat4("u_View", camera->GetViewMatrix());
    renderer->SetUniformMat4("u_Projection", camera->GetProjectionMatrix());
    for(int i=0;i<6;i++){
        Mat4 m=glm::translate(Mat4(1.0f), Vec3(parts[i].x,parts[i].y,parts[i].z)+playerPos);
        m=glm::scale(m, Vec3(parts[i].sx,parts[i].sy,parts[i].sz));
        renderer->SetUniformMat4("u_Model", m);
        renderer->SetUniformVec4("u_Color", Vec4(parts[i].r,parts[i].g,parts[i].b,1.0f));
        renderer->DrawMesh(m_playerCubeMesh);
    }
}
void VoxelGame::RenderBlockHighlight(Renderer* renderer, Camera* camera) {"""
c=c.replace(old_func,new_func,1)
print("5. RenderPlayerModel function added")

# Add cleanup
old_clean="        if (m_wireframeFS != kInvalidHandle) r->DestroyShader(m_wireframeFS);"
new_clean="""        if (m_wireframeFS != kInvalidHandle) r->DestroyShader(m_wireframeFS);
        if (m_playerCubeMesh != kInvalidHandle) r->DestroyMesh(m_playerCubeMesh);
        if (m_playerPipeline != kInvalidHandle) r->DestroyPipelineState(m_playerPipeline);
        if (m_playerVS != kInvalidHandle) r->DestroyShader(m_playerVS);
        if (m_playerFS != kInvalidHandle) r->DestroyShader(m_playerFS);"""
c=c.replace(old_clean,new_clean,1)
print("6. Cleanup added")

open("G:/AI_projects/GameEngine/src/game/VoxelGame.cpp","w",encoding="utf-8").write(c)
print("All changes applied!")
