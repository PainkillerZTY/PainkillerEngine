
import re
f=open('G:/AI_projects/GameEngine/src/game/VoxelGame.cpp','r',encoding='utf-8');c=f.read();f.close()

# Fix getFaceUV - use targeted find for each line
i=c.find('getFaceUV')
j=c.find('// Water a',i)
func=c[i:j]

# Case 0: f==0
func=func.replace('1.0 - l.y);   // +X Right','l.y);       // +X Right',1)
# Case 1: f==1
func=func.replace('1.0 - l.y);     // -X Left','l.y);         // -X Left',1)
# Case 4: f==4
func=func.replace('1.0 - l.y); // +Z Front','l.y);    // +Z Front',1)
# Case 5: default
func=func.replace('1.0 - l.y);                   // -Z Back','l.y);                      // -Z Back',1)

c=c[:i]+func+c[j:]
print('1. getFaceUV fixed')

# Verify
i2=c.find('getFaceUV')
if 'l.y);' in c[i2:i2+400]:
    print('  WARNING: l.y); still found in getFaceUV')
else:
    print('  l.y); removed from getFaceUV')

# Replace HandleBlockInteraction
old_hbi = c[c.find('void VoxelGame::HandleBlockInteraction'):c.find('if (input->IsKeyPressed(VK_ESCAPE))',c.find('void VoxelGame::HandleBlockInteraction'))]
old_hbi = old_hbi[:old_hbi.find('if (input->IsKeyPressed(VK_ESCAPE))')]
old_hbi = old_hbi[:old_hbi.rfind('}')+1]
# Find the ending }
end_hbi = c.find('}

    if (input->IsKeyPressed(VK_ESCAPE)',c.find('HandleBlockInteraction'))
print('2. HBI section found at', end_hbi)
with open('G:/AI_projects/GameEngine/src/game/VoxelGame.cpp','w',encoding='utf-8') as f:
    f.write(c)
print('Saved')
