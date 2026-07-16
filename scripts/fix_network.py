import os
g = "G:/AI_projects/GameEngine/src/game"
cml = g + "/CMakeLists.txt"

# 1. Create Packet.h
with open(g+"/Packet.h","w") as f:
    f.write('#pragma once\n#include "Types.h"\n#include <vector>\n#include <string>\nnamespace painkiller {\nclass Packet {\npublic:\n    Packet();\n    void WriteU8(u8 v);\n    void WriteU32(u32 v);\n    void WriteF32(f32 v);\n    void WriteString(const std::string& s);\n    u8 ReadU8();\n    u32 ReadU32();\n    f32 ReadF32();\n    std::string ReadString();\n    const std::vector<u8>& GetData() const { return m_data; }\n    void SetData(const u8* data, u32 size);\n    void Clear() { m_data.clear(); m_readPos = 0; }\nprivate:\n    std::vector<u8> m_data;\n    u32 m_readPos = 0;\n};\n}\n')
print("Packet.h created")

# 2. Create Network.h
with open(g+"/Network.h","w") as f:
    f.write('#pragma once\n#include "Types.h"\n#include "Packet.h"\n#include <functional>\n#define WIN32_LEAN_AND_MEAN\n#include <winsock2.h>\nnamespace painkiller {\nclass NetworkServer {\npublic:\n    NetworkServer();\n    ~NetworkServer();\n    bool Start(u16 port);\n    void Stop();\n    void Update();\n    bool IsRunning() const { return m_running; }\n    void SetOnClientConnect(std::function<void(int)> cb) { m_onConnect = cb; }\n    void SetOnClientDisconnect(std::function<void(int)> cb) { m_onDisconnect = cb; }\nprivate:\n    SOCKET m_listenSocket = INVALID_SOCKET;\n    bool m_running = false;\n    std::function<void(int)> m_onConnect;\n    std::function<void(int)> m_onDisconnect;\n};\nclass NetworkClient {\npublic:\n    NetworkClient();\n    ~NetworkClient();\n    bool Connect(const char* host, u16 port);\n    void Disconnect();\n    bool IsConnected() const { return m_connected; }\nprivate:\n    SOCKET m_socket = INVALID_SOCKET;\n    bool m_connected = false;\n};\n}\n')
print("Network.h created")

# 3. Create Packet.cpp
with open(g+"/Packet.cpp","w") as f:
    f.write('#include "Packet.h"\n#include <cstring>\nnamespace painkiller {\nPacket::Packet(){}\nvoid Packet::WriteU8(u8 v){m_data.push_back(v);}\nvoid Packet::WriteU32(u32 v){m_data.push_back((v>>24)&0xFF);m_data.push_back((v>>16)&0xFF);m_data.push_back((v>>8)&0xFF);m_data.push_back(v&0xFF);}\nvoid Packet::WriteF32(f32 v){u32*p=(u32*)&v;WriteU32(*p);}\nvoid Packet::WriteString(const std::string& s){WriteU32((u32)s.size());for(char c:s)m_data.push_back((u8)c);}\nu8 Packet::ReadU8(){if(m_readPos>=m_data.size())return 0;return m_data[m_readPos++];}\nu32 Packet::ReadU32(){return((u32)ReadU8()<<24)|((u32)ReadU8()<<16)|((u32)ReadU8()<<8)|ReadU8();}\nf32 Packet::ReadF32(){u32 v=ReadU32();return *(f32*)&v;}\nstd::string Packet::ReadString(){u32 l=ReadU32();std::string s;for(u32 i=0;i<l;i++)s+=(char)ReadU8();return s;}\nvoid Packet::SetData(const u8* d,u32 s){m_data.assign(d,d+s);m_readPos=0;}\n}\n')
print("Packet.cpp created")

# 4. Create Network.cpp
cpp = '#define WIN32_LEAN_AND_MEAN\n#include <winsock2.h>\n#include "Network.h"\n#include "Logger.h"\n#include <cstdio>\nnamespace painkiller {\n'
cpp += 'NetworkServer::NetworkServer(){}\nNetworkServer::~NetworkServer(){Stop();}\n'
cpp += 'bool NetworkServer::Start(u16 port){\n#ifdef _WIN32\nWSADATA wsa;WSAStartup(MAKEWORD(2,2),&wsa);\n'
cpp += 'm_listenSocket=socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);\nif(m_listenSocket==INVALID_SOCKET)return false;\n'
cpp += 'sockaddr_in addr;addr.sin_family=AF_INET;addr.sin_addr.s_addr=INADDR_ANY;addr.sin_port=htons(port);\n'
cpp += 'if(bind(m_listenSocket,(sockaddr*)&addr,sizeof(addr))==SOCKET_ERROR){closesocket(m_listenSocket);return false;}\n'
cpp += 'listen(m_listenSocket,SOMAXCONN);u_long m=1;ioctlsocket(m_listenSocket,FIONBIO,&m);\n'
cpp += 'm_running=true;LOG_INFO("Server on port {}",port);\n#endif\nreturn true;}\n'
cpp += 'void NetworkServer::Stop(){m_running=false;\n#ifdef _WIN32\nif(m_listenSocket!=INVALID_SOCKET){closesocket(m_listenSocket);m_listenSocket=INVALID_SOCKET;}WSACleanup();\n#endif\n}\n'
cpp += 'void NetworkServer::Update(){\n#ifdef _WIN32\nif(!m_running)return;\nSOCKET c=accept(m_listenSocket,0,0);\nif(c!=INVALID_SOCKET){u_long m=1;ioctlsocket(c,FIONBIO,&m);LOG_INFO("Client connected");if(m_onConnect)m_onConnect((int)c);}\n#endif\n}\n'
cpp += 'NetworkClient::NetworkClient(){}\nNetworkClient::~NetworkClient(){Disconnect();}\n'
cpp += 'bool NetworkClient::Connect(const char* host,u16 port){\n#ifdef _WIN32\nWSADATA wsa;WSAStartup(MAKEWORD(2,2),&wsa);\n'
cpp += 'm_socket=socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);\nif(m_socket==INVALID_SOCKET)return false;\n'
cpp += 'sockaddr_in addr;addr.sin_family=AF_INET;addr.sin_addr.s_addr=inet_addr(host);addr.sin_port=htons(port);\n'
cpp += 'if(connect(m_socket,(sockaddr*)&addr,sizeof(addr))==SOCKET_ERROR){closesocket(m_socket);m_socket=INVALID_SOCKET;return false;}\n'
cpp += 'u_long m=1;ioctlsocket(m_socket,FIONBIO,&m);m_connected=true;LOG_INFO("Connected to {}:{}",host,port);\n#endif\nreturn true;}\n'
cpp += 'void NetworkClient::Disconnect(){m_connected=false;\n#ifdef _WIN32\nif(m_socket!=INVALID_SOCKET){closesocket(m_socket);m_socket=INVALID_SOCKET;}WSACleanup();\n#endif\n}\n}\n'
with open(g+"/Network.cpp","w") as f:
    f.write(cpp)
print("Network.cpp created")

# 5. Modify CMakeLists.txt
cm = open(cml,"r",encoding="utf-8").read()
if "Packet.h" not in cm:
    cm = cm.replace("ParticleSystem.h ParticleSystem.cpp",
                     "Packet.h Packet.cpp\n    Network.h Network.cpp\n    ParticleSystem.h ParticleSystem.cpp", 1)
    cm = cm.replace("painkiller PRIVATE painkiller_engine",
                     "painkiller PRIVATE painkiller_engine ws2_32", 1)
    open(cml,"w",encoding="utf-8").write(cm)
    print("CMakeLists.txt updated")
else:
    print("CMakeLists.txt already has network files")

print("ALL DONE!")
