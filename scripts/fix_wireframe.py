import re
# Read and modify VoxelGame.h
h=open("G:/AI_projects/GameEngine/src/game/VoxelGame.h","r",encoding="utf-8").read()
h=h.replace("MeshHandle m_skyboxMesh = kInvalidHandle;\n\n    // State",
         "MeshHandle m_skyboxMesh = kInvalidHandle;\n    ShaderHandle m_wireframeVS = kInvalidHandle;\n    ShaderHandle m_wireframeFS = kInvalidHandle;\n    PipelineHandle m_wireframePipeline = kInvalidHandle;\n    MeshHandle m_wireframeCubeMesh = kInvalidHandle;\n\n    // State",1)
h=h.replace("void RenderHeldBlock(Renderer* renderer, Camera* camera);",
         "void RenderHeldBlock(Renderer* renderer, Camera* camera);\n    void RenderBlockHighlight(Renderer* renderer, Camera* camera);",1)
open("G:/AI_projects/GameEngine/src/game/VoxelGame.h","w",encoding="utf-8").write(h)
print("Header updated")

# Read VoxelGame.cpp
c=open("G:/AI_projects/GameEngine/src/game/VoxelGame.cpp","r",encoding="utf-8").read()

# 1. Add wireframe pipeline after skybox pipeline
old_pipe = "    }\n    \n    m_pipelineCreated = true;"
new_pipe = """    }
    // Wireframe pipeline for block highlight (LINE_LIST)
    {
        const char* wfVS = R"(#version 330 core
            layout(location=0) in vec3 a_Position;
            uniform mat4 u_View; uniform mat4 u_Projection;
            void main() { gl_Position = u_Projection * u_View * vec4(a_Position, 1.0); }
        )";
        const char* wfFS = R"(#version 330 core
            uniform vec4 u_Color; out vec4 FragColor;
            void main() { FragColor = u_Color; }
        )";
        ShaderDesc wvs; wvs.stage=ShaderStage::Vertex; wvs.source=wfVS;
        ShaderDesc wfs; wfs.stage=ShaderStage::Fragment; wfs.source=wfFS;
        m_wireframeVS=renderer->CreateShader(wvs);
        m_wireframeFS=renderer->CreateShader(wfs);
        if(m_wireframeVS!=kInvalidHandle && m_wireframeFS!=kInvalidHandle){
            PipelineStateDesc wp;
            wp.vertexShader=&wvs; wp.fragmentShader=&wfs;
            wp.topology=PrimitiveTopology::LineList;
            wp.cullMode=CullMode::None;
            wp.depthTestEnabled=true; wp.depthWriteEnabled=false;
            wp.depthFunc=CompareFunction::Less;
            wp.vertexLayout={{"POSITION",Format::R32G32B32_FLOAT,0,0}};
            m_wireframePipeline=renderer->CreatePipelineState(wp);
        }
    }
    
    m_pipelineCreated = true;"""
c=c.replace(old_pipe,new_pipe,1)
print("1. Wireframe pipeline added")

# 2. Add wireframe cube mesh after skybox mesh
old_mesh="        m_skyboxMesh = renderer->CreateMesh(sb);\n    }"
new_mesh="""        m_skyboxMesh = renderer->CreateMesh(sb);
    }
    // 9. Wireframe cube mesh for block highlight (12 lines, LINE_LIST)
    {
        MeshData wf;
        wf.vertexStride = 3*sizeof(f32);
        f32 wfVerts[] = {
            -0.5f,-0.5f,-0.5f, 0.5f,-0.5f,-0.5f, 0.5f, 0.5f,-0.5f, -0.5f, 0.5f,-0.5f,
            -0.5f,-0.5f, 0.5f, 0.5f,-0.5f, 0.5f, 0.5f, 0.5f, 0.5f, -0.5f, 0.5f, 0.5f
        };
        wf.vertices.assign(wfVerts, wfVerts+24);
        wf.vertexCount = 8;
        u32 wfIdx[] = {0,1, 1,2, 2,3, 3,0, 4,5, 5,6, 6,7, 7,4, 0,4, 1,5, 2,6, 3,7};
        wf.indices.assign(wfIdx, wfIdx+24);
        wf.indexCount = 24;
        m_wireframeCubeMesh = renderer->CreateMesh(wf);
    }"""
c=c.replace(old_mesh,new_mesh,1)
print("2. Wireframe cube mesh added")

# 3. Add RenderBlockHighlight call in Render function
old_render_end="        RenderHeldBlock(renderer, camera);\n    }\n    \n    // Always render UI"
new_render_end="""        RenderHeldBlock(renderer, camera);
    }
    // Block highlight wireframe
    RenderBlockHighlight(renderer, camera);
    
    // Always render UI"""
c=c.replace(old_render_end,new_render_end,1)
print("3. RenderBlockHighlight call added")

# 4. Add RenderBlockHighlight function before Shutdown
old_shutdown="void VoxelGame::Shutdown(Engine* engine) {"
new_func="""void VoxelGame::RenderBlockHighlight(Renderer* renderer, Camera* camera) {
    if (m_wireframePipeline == kInvalidHandle || m_wireframeCubeMesh == kInvalidHandle) return;
    if (!m_raycastResult.hit) return;
    Vec3 blockPos((f32)m_raycastResult.blockX + 0.5f, (f32)m_raycastResult.blockY + 0.5f, (f32)m_raycastResult.blockZ + 0.5f);
    renderer->BindPipelineState(m_wireframePipeline);
    renderer->SetUniformMat4("u_View", camera->GetViewMatrix());
    renderer->SetUniformMat4("u_Projection", camera->GetProjectionMatrix());
    Mat4 model = glm::translate(Mat4(1.0f), blockPos);
    renderer->SetUniformMat4("u_Model", model);
    renderer->SetUniformVec4("u_Color", Vec4(0.0f, 0.0f, 0.0f, 1.0f));
    renderer->DrawMesh(m_wireframeCubeMesh);
    // Outer highlight
    model = glm::scale(glm::translate(Mat4(1.0f), blockPos), Vec3(1.02f));
    renderer->SetUniformMat4("u_Model", model);
    renderer->SetUniformVec4("u_Color", Vec4(1.0f, 1.0f, 1.0f, 0.8f));
    renderer->DrawMesh(m_wireframeCubeMesh);
}
void VoxelGame::Shutdown(Engine* engine) {"""
c=c.replace(old_shutdown,new_func,1)
print("4. RenderBlockHighlight function added")

# 5. Add cleanup in Shutdown
old_clean="        if (m_skyboxFS != kInvalidHandle) r->DestroyShader(m_skyboxFS);"
new_clean="""        if (m_skyboxFS != kInvalidHandle) r->DestroyShader(m_skyboxFS);
        if (m_wireframeCubeMesh != kInvalidHandle) r->DestroyMesh(m_wireframeCubeMesh);
        if (m_wireframePipeline != kInvalidHandle) r->DestroyPipelineState(m_wireframePipeline);
        if (m_wireframeVS != kInvalidHandle) r->DestroyShader(m_wireframeVS);
        if (m_wireframeFS != kInvalidHandle) r->DestroyShader(m_wireframeFS);"""
c=c.replace(old_clean,new_clean,1)
print("5. Cleanup added")

open("G:/AI_projects/GameEngine/src/game/VoxelGame.cpp","w",encoding="utf-8").write(c)
print("All changes applied!")
