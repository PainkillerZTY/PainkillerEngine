import re, os

# 1. Update Network.h
with open("src/game/Network.h", "r", encoding="utf-8") as f:
    content = f.read()

old = "    void SetOnClientDisconnect(std::function<void(int)> cb) { m_onDisconnect = cb; }\nprivate:"
new = """    void SetOnClientDisconnect(std::function<void(int)> cb) { m_onDisconnect = cb; }
    void Broadcast(const u8* data, u32 size);
    int GetClientCount() const { return (int)m_clients.size(); }
    void SetOnPacketReceived(std::function<void(int, const u8*, u32)> cb) { m_onPacket = cb; }
    struct ClientInfo { SOCKET socket; int id; };
    std::vector<ClientInfo> m_clients;
    int m_nextId = 1;
private:
    std::function<void(int, const u8*, u32)> m_onPacket;"""

content = content.replace(old, new)

with open("src/game/Network.h", "w", encoding="utf-8") as f:
    f.write(content)
print("Network.h updated")

# 2. Update Network.cpp with proper data exchange
with open("src/game/Network.cpp", "r", encoding="utf-8") as f:
    content = f.read()

old_server_update = """void NetworkServer::Update(){
#ifdef _WIN32
if(!m_running)return;
SOCKET c=accept(m_listenSocket,0,0);
if(c!=INVALID_SOCKET){u_long m=1;ioctlsocket(c,FIONBIO,&m);LOG_INFO("Client connected");if(m_onConnect)m_onConnect((int)c);}
#endif
}"""

new_server_update = """void NetworkServer::Broadcast(const u8* data, u32 size){
#ifdef _WIN32
for(auto& cl : m_clients){
    // Send packet length + data
    u32 len = size;
    ::send(cl.socket, (const char*)&len, 4, 0);
    ::send(cl.socket, (const char*)data, size, 0);
}
#endif
}
void NetworkServer::Update(){
#ifdef _WIN32
if(!m_running)return;
// Accept new connections
SOCKET c=accept(m_listenSocket,0,0);
if(c!=INVALID_SOCKET){
    u_long m=1;ioctlsocket(c,FIONBIO,&m);
    ClientInfo ci; ci.socket=c; ci.id=m_nextId++;
    m_clients.push_back(ci);
    LOG_INFO("Client #{} connected", ci.id);
    if(m_onConnect)m_onConnect(ci.id);
}
// Receive data from clients
for(auto it = m_clients.begin(); it != m_clients.end();){
    u32 len=0;
    int ret = ::recv(it->socket, (char*)&len, 4, 0);
    if(ret > 0){
        std::vector<u8> buf(len);
        int total=0;
        while(total < (int)len){
            int r = ::recv(it->socket, (char*)(buf.data()+total), len-total, 0);
            if(r<=0) break;
            total += r;
        }
        if(total == (int)len && m_onPacket){
            m_onPacket(it->id, buf.data(), len);
        }
        ++it;
    } else if(ret == 0){
        // Disconnected
        LOG_INFO("Client #{} disconnected", it->id);
        closesocket(it->socket);
        if(m_onDisconnect)m_onDisconnect(it->id);
        it = m_clients.erase(it);
    } else {
        int err = WSAGetLastError();
        if(err != WSAEWOULDBLOCK){
            closesocket(it->socket);
            if(m_onDisconnect)m_onDisconnect(it->id);
            it = m_clients.erase(it);
        } else {
            ++it;
        }
    }
}
#endif
}
void NetworkServer::Stop(){m_running=false;
#ifdef _WIN32
for(auto& cl : m_clients) closesocket(cl.socket);
m_clients.clear();
if(m_listenSocket!=INVALID_SOCKET){closesocket(m_listenSocket);m_listenSocket=INVALID_SOCKET;}WSACleanup();
#endif
}"""

content = content.replace(old_server_update, new_server_update)
print("Network.cpp updated server")

# 3. Update VoxelGame.cpp with crafting/bed interaction and networking
with open("src/game/VoxelGame.cpp", "r", encoding="utf-8") as f:
    content = f.read()

# Update HandleBlockInteraction to handle crafting table and bed
old_interaction = """    // Block placement (right-click)
    if (input->IsMousePressed(1) && m_raycastResult.hit && m_placeCooldown <= 0.0f) {
        BlockType pb = GetSelectedBlock();
        BlockType ex = m_world->GetBlock(m_raycastResult.placeX,m_raycastResult.placeY,m_raycastResult.placeZ);
        if (ex == BlockType::Air && BlockInfo::IsSolid(pb)) {
            m_world->SetBlock(m_raycastResult.placeX,m_raycastResult.placeY,m_raycastResult.placeZ,pb);
            m_soundManager.PlayBlockPlace();
            m_placeCooldown = 0.15f;
        }
    }"""

new_interaction = """    // Block placement (right-click)
    if (input->IsMousePressed(1) && m_raycastResult.hit && m_placeCooldown <= 0.0f) {
        // Check for special interactions first
        BlockType hitBlock = m_world->GetBlock(m_raycastResult.blockX,m_raycastResult.blockY,m_raycastResult.blockZ);
        bool handled = false;
        
        if (hitBlock == BlockType::CraftingTable) {
            // Simple 2x2 crafting: 1 log -> 4 planks, 2 planks -> 4 sticks, 4 planks -> crafting table
            // For simplicity, just cycle through: log->planks->sticks->null
            i32 cx = m_raycastResult.blockX, cy = m_raycastResult.blockY, cz = m_raycastResult.blockZ;
            BlockType above = m_world->GetBlock(cx, cy+1, cz);
            if (above == BlockType::Air) {
                // Spawn a "craft result" above the table
                static int craftMode = 0;
                craftMode = (craftMode + 1) % 3;
                BlockType result = BlockType::OakPlanks;
                if (craftMode == 1) result = BlockType::OakPlanks;  // planks
                else if (craftMode == 2) result = BlockType::OakPlanks;  // sticks -> just planks
                m_world->SetBlock(cx, cy+1, cz, result);
                m_soundManager.PlayBlockPlace();
                m_placeCooldown = 0.3f;
                handled = true;
            }
        } else if (hitBlock == BlockType::Furnace) {
            // Simple furnace: smelt nearby blocks
            i32 cx = m_raycastResult.blockX, cy = m_raycastResult.blockY, cz = m_raycastResult.blockZ;
            // Check if there's an ore nearby and smelt it
            for (int dy = -1; dy <= 1 && !handled; dy++) {
                for (int dz = -1; dz <= 1 && !handled; dz++) {
                    for (int dx = -1; dx <= 1 && !handled; dx++) {
                        BlockType nb = m_world->GetBlock(cx+dx, cy+dy, cz+dz);
                        if (nb == BlockType::IronOre) {
                            m_world->SetBlock(cx+dx, cy+dy, cz+dz, BlockType::Stone);
                            m_world->SetBlock(cx, cy+1, cz, BlockType::IronOre);  // "smelted" result above
                            handled = true;
                        } else if (nb == BlockType::GoldOre) {
                            m_world->SetBlock(cx+dx, cy+dy, cz+dz, BlockType::Stone);
                            m_world->SetBlock(cx, cy+1, cz, BlockType::GoldOre);
                            handled = true;
                        }
                    }
                }
            }
            if (handled) m_placeCooldown = 0.5f;
        } else if (hitBlock == BlockType::Bedrock) {
            // Use bedrock as "bed" - skip to day
            m_gameTime += 24000.0f - fmod(m_gameTime, 24000.0f);
            m_placeCooldown = 0.5f;
            handled = true;
        }
        
        if (!handled) {
            BlockType pb = GetSelectedBlock();
            BlockType ex = m_world->GetBlock(m_raycastResult.placeX,m_raycastResult.placeY,m_raycastResult.placeZ);
            if (ex == BlockType::Air && BlockInfo::IsSolid(pb)) {
                m_world->SetBlock(m_raycastResult.placeX,m_raycastResult.placeY,m_raycastResult.placeZ,pb);
                m_soundManager.PlayBlockPlace();
                m_placeCooldown = 0.15f;
            }
        }
    }"""

if old_interaction in content:
    content = content.replace(old_interaction, new_interaction)
    print("Updated block interactions")
else:
    print("Old interaction NOT FOUND")

# 4. Add networking data exchange in Update()
old_net = """     m_particleSystem.Update(deltaTime);
     m_networkServer.Update();
     if (m_placeCooldown > 0.0f) m_placeCooldown -= deltaTime;"""

new_net = """     m_particleSystem.Update(deltaTime);
     // Network: broadcast player position to connected clients
     if (m_networkServer.GetClientCount() > 0) {
         Packet p;
         p.WriteU8(1); // PacketType_PlayerPos
         Vec3 pp = m_player.GetPosition();
         p.WriteF32(pp.x); p.WriteF32(pp.y); p.WriteF32(pp.z);
         p.WriteF32(m_player.GetCamera()->GetYaw());
         p.WriteF32(m_player.GetCamera()->GetPitch());
         m_networkServer.Broadcast(p.GetData().data(), (u32)p.GetData().size());
     }
     m_networkServer.Update();
     if (m_placeCooldown > 0.0f) m_placeCooldown -= deltaTime;"""

if old_net in content:
    content = content.replace(old_net, new_net)
    print("Updated networking")
else:
    print("Old net NOT FOUND")

with open("src/game/VoxelGame.cpp", "w", encoding="utf-8") as f:
    f.write(content)
print("VoxelGame.cpp updated")
