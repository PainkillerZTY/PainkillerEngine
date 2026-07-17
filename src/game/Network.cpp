#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include "Network.h"
#include "Logger.h"
#include <cstdio>
#include <vector>
namespace painkiller {
NetworkServer::NetworkServer(){}
NetworkServer::~NetworkServer(){Stop();}
bool NetworkServer::Start(u16 port){
#ifdef _WIN32
WSADATA wsa;WSAStartup(MAKEWORD(2,2),&wsa);
m_listenSocket=socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
if(m_listenSocket==INVALID_SOCKET)return false;
sockaddr_in addr;addr.sin_family=AF_INET;addr.sin_addr.s_addr=INADDR_ANY;addr.sin_port=htons(port);
if(bind(m_listenSocket,(sockaddr*)&addr,sizeof(addr))==SOCKET_ERROR){closesocket(m_listenSocket);return false;}
listen(m_listenSocket,SOMAXCONN);u_long m=1;ioctlsocket(m_listenSocket,FIONBIO,&m);
m_running=true;LOG_INFO("Server on port {}",port);
#endif
return true;}
void NetworkServer::Broadcast(const u8* data, u32 size){
#ifdef _WIN32
for(auto& cl : m_clients){
    u32 len = size;
    ::send(cl.socket, (const char*)&len, 4, 0);
    ::send(cl.socket, (const char*)data, size, 0);
}
#endif
}
void NetworkServer::SendTo(int clientId, const u8* data, u32 size){
#ifdef _WIN32
for(auto& cl : m_clients){
    if(cl.id == clientId){
        u32 len = size;
        ::send(cl.socket, (const char*)&len, 4, 0);
        ::send(cl.socket, (const char*)data, size, 0);
        return;
    }
}
#endif
}
void NetworkServer::Update(){
#ifdef _WIN32
if(!m_running)return;
SOCKET c=accept(m_listenSocket,0,0);
if(c!=INVALID_SOCKET){
    u_long m=1;ioctlsocket(c,FIONBIO,&m);
    ClientInfo ci; ci.socket=c; ci.id=m_nextId++;
    m_clients.push_back(ci);
    LOG_INFO("Client #{} connected", ci.id);
    if(m_onConnect)m_onConnect(ci.id);
}
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
        closesocket(it->socket);
        if(m_onDisconnect)m_onDisconnect(it->id);
        it = m_clients.erase(it);
    } else {
        int err = WSAGetLastError();
        if(err != WSAEWOULDBLOCK){
            closesocket(it->socket);
            if(m_onDisconnect)m_onDisconnect(it->id);
            it = m_clients.erase(it);
        } else { ++it; }
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
}
NetworkClient::NetworkClient(){}
NetworkClient::~NetworkClient(){Disconnect();}
bool NetworkClient::Connect(const char* host,u16 port){
#ifdef _WIN32
WSADATA wsa;WSAStartup(MAKEWORD(2,2),&wsa);
m_socket=socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
if(m_socket==INVALID_SOCKET)return false;
sockaddr_in addr;addr.sin_family=AF_INET;addr.sin_addr.s_addr=inet_addr(host);addr.sin_port=htons(port);
if(connect(m_socket,(sockaddr*)&addr,sizeof(addr))==SOCKET_ERROR){closesocket(m_socket);m_socket=INVALID_SOCKET;return false;}
u_long m=1;ioctlsocket(m_socket,FIONBIO,&m);m_connected=true;LOG_INFO("Connected to {}:{}",host,port);
#endif
return true;}
void NetworkClient::Disconnect(){m_connected=false;
#ifdef _WIN32
if(m_socket!=INVALID_SOCKET){closesocket(m_socket);m_socket=INVALID_SOCKET;}WSACleanup();
#endif
}
void NetworkClient::Update(){
#ifdef _WIN32
if(!m_connected || m_socket == INVALID_SOCKET) return;
// Non-blocking recv loop
u8 buf[4096];
int ret = ::recv(m_socket, (char*)buf, sizeof(buf), 0);
if(ret > 0){
    // Append to recv buffer
    m_recvBuf.insert(m_recvBuf.end(), buf, buf + ret);
    // Process as many complete packets as we can
    while(true){
        if(m_recvState == 0){
            // Waiting for 4-byte length prefix
            if(m_recvBuf.size() < 4) break;
            m_pendingLen = ((u32)m_recvBuf[0]<<24)|((u32)m_recvBuf[1]<<16)|((u32)m_recvBuf[2]<<8)|(u32)m_recvBuf[3];
            m_recvBuf.erase(m_recvBuf.begin(), m_recvBuf.begin() + 4);
            m_recvState = 1;
            m_pendingData.clear();
        }
        if(m_recvState == 1){
            // Waiting for payload
            if(m_recvBuf.size() < m_pendingLen) break;
            m_pendingData.assign(m_recvBuf.begin(), m_recvBuf.begin() + m_pendingLen);
            m_recvBuf.erase(m_recvBuf.begin(), m_recvBuf.begin() + m_pendingLen);
            m_recvState = 0;
            if(m_onPacket)
                m_onPacket(m_pendingData.data(), (u32)m_pendingData.size());
        }
    }
} else if(ret == 0){
    // Clean disconnect
    m_connected = false;
    closesocket(m_socket); m_socket = INVALID_SOCKET;
    if(m_onDisconnect) m_onDisconnect();
} else {
    int err = WSAGetLastError();
    if(err != WSAEWOULDBLOCK){
        m_connected = false;
        closesocket(m_socket); m_socket = INVALID_SOCKET;
        if(m_onDisconnect) m_onDisconnect();
    }
}
#endif
}
void NetworkClient::Send(const u8* data, u32 size){
#ifdef _WIN32
if(!m_connected || m_socket == INVALID_SOCKET) return;
u32 len = size;
::send(m_socket, (const char*)&len, 4, 0);
::send(m_socket, (const char*)data, size, 0);
#endif
}
}
