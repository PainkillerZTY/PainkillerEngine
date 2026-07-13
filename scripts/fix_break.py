import re
c=open("G:/AI_projects/GameEngine/src/game/VoxelGame.cpp","r",encoding="utf-8").read()

# 1. Find HBI function boundaries
hbi_sig = "void VoxelGame::HandleBlockInteraction(Engine* engine)"
i = c.find(hbi_sig)
j = c.find("if (input->IsKeyPressed(VK_ESCAPE))", i)
# Find the closing brace BEFORE the escape check
j = c.rfind("}", i, j) + 1
old_hbi = c[i:j]

# 2. New HBI with break timer
lines = []
lines.append("void VoxelGame::HandleBlockInteraction(Engine* engine, f32 deltaTime) {")
lines.append("    auto* input = engine->GetInput();")
lines.append("    auto* camera = m_player.GetCamera();")
lines.append("    Vec3 eyePos = m_player.GetPosition();")
lines.append("    Vec3 forward = camera->GetForward();")
lines.append("    m_raycastResult = BlockRaycast::Cast(eyePos, forward, 8.0f, m_world);")
lines.append("")
lines.append("    // Block breaking with timer delay")
lines.append("    if (input->IsMouseDown(0) && m_raycastResult.hit) {")
lines.append("        BlockType block = m_world->GetBlock(m_raycastResult.blockX, m_raycastResult.blockY, m_raycastResult.blockZ);")
lines.append("        if (block != BlockType::Air && block != BlockType::Water) {")
lines.append("            if (!m_isBreaking || m_breakX != m_raycastResult.blockX ||")
lines.append("                m_breakY != m_raycastResult.blockY || m_breakZ != m_raycastResult.blockZ) {")
lines.append("                m_isBreaking = true; m_breakTimer = 0.0f;")
lines.append("                m_breakX = m_raycastResult.blockX; m_breakY = m_raycastResult.blockY; m_breakZ = m_raycastResult.blockZ;")
lines.append("            }")
lines.append("            f32 h = BlockInfo::GetHardness(block); if (h <= 0.0f) h = 0.3f;")
lines.append("            m_breakTimer += deltaTime;")
lines.append("            if (m_breakTimer >= h) {")
lines.append("                Vec3 wp((f32)m_breakX+0.5f,(f32)m_breakY+0.5f,(f32)m_breakZ+0.5f);")
lines.append("                Vec3 bc = BlockInfo::GetFaceColor(block, BlockFace::Top);")
lines.append("                m_world->SetBlock(m_breakX,m_breakY,m_breakZ,BlockType::Air);")
lines.append("                m_particleSystem.SpawnBreakParticles(wp,bc,12);")
lines.append("                m_soundManager.PlayBlockBreak();")
lines.append("                m_isBreaking = false; m_breakTimer = 0.0f;")
lines.append("            }")
lines.append("        } else { m_isBreaking = false; m_breakTimer = 0.0f; }")
lines.append("    } else { m_isBreaking = false; m_breakTimer = 0.0f; }")
lines.append("")
lines.append("    // Block placement (right-click)")
lines.append("    if (input->IsMousePressed(1) && m_raycastResult.hit && m_placeCooldown <= 0.0f) {")
lines.append("        BlockType pb = GetSelectedBlock();")
lines.append("        BlockType ex = m_world->GetBlock(m_raycastResult.placeX,m_raycastResult.placeY,m_raycastResult.placeZ);")
lines.append("        if (ex == BlockType::Air && BlockInfo::IsSolid(pb)) {")
lines.append("            m_world->SetBlock(m_raycastResult.placeX,m_raycastResult.placeY,m_raycastResult.placeZ,pb);")
lines.append("            m_soundManager.PlayBlockPlace();")
lines.append("            m_placeCooldown = 0.15f;")
lines.append("        }")
lines.append("    }")
lines.append("    if (input->IsKeyPressed(VK_ESCAPE)) engine->Stop();")
lines.append("}")
new_hbi = "\n".join(lines)
c = c.replace(old_hbi, new_hbi, 1)
print("1. HBI updated")

# 3. Update call site
c = c.replace("HandleBlockInteraction(engine);", "HandleBlockInteraction(engine, deltaTime);", 1)
print("2. Call site updated")

open("G:/AI_projects/GameEngine/src/game/VoxelGame.cpp","w",encoding="utf-8").write(c)
print("3. Saved")

# 4. Verify
c2 = open("G:/AI_projects/GameEngine/src/game/VoxelGame.cpp","r",encoding="utf-8").read()
if "f32 deltaTime" in c2[c2.find("void VoxelGame::HandleBlockInteraction"):c2.find("void VoxelGame::HandleBlockInteraction")+500]:
    print("4. deltaTime param VERIFIED")
else:
    print("4. deltaTime param MISSING!")
