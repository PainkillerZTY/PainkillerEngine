import os, re
g = "G:/AI_projects/GameEngine/src/game"

# VoxelGame.h - Add include and member
h = open(g+"/VoxelGame.h","r",encoding="utf-8").read()
h = re.sub(r'#include "SoundManager.h"', '#include "SoundManager.h"\n#include "Network.h"', h, 1)
h = re.sub(r'SoundManager m_soundManager;', 'SoundManager m_soundManager;\n    NetworkServer m_networkServer;', h, 1)
open(g+"/VoxelGame.h","w",encoding="utf-8").write(h)
print("VoxelGame.h done")

# VoxelGame.cpp - Add start, update, stop
c = open(g+"/VoxelGame.cpp","r",encoding="utf-8").read()
c = c.replace("m_soundManager.Initialize();", "m_soundManager.Initialize();\n    m_networkServer.Start(25565);", 1)
c = c.replace("m_particleSystem.Update(deltaTime);\n     if (m_placeCooldown > 0.0f)",
              "m_particleSystem.Update(deltaTime);\n     m_networkServer.Update();\n     if (m_placeCooldown > 0.0f)", 1)
c = c.replace("m_soundManager.Shutdown();", "m_networkServer.Stop();\n    m_soundManager.Shutdown();", 1)
open(g+"/VoxelGame.cpp","w",encoding="utf-8").write(c)
print("VoxelGame.cpp done")
print("ALL DONE!")
