#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include "Network.h"
#include "Logger.h"
#include <cstdio>
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
void NetworkServer::Stop(){m_running=false;
#ifdef _WIN32
if(m_listenSocket!=INVALID_SOCKET){closesocket(m_listenSocket);m_listenSocket=INVALID_SOCKET;}WSACleanup();
#endif
}
void NetworkServer::Update(){
#ifdef _WIN32
if(!m_running)return;
SOCKET c=accept(m_listenSocket,0,0);
if(c!=INVALID_SOCKET){u_long m=1;ioctlsocket(c,FIONBIO,&m);LOG_INFO("Client connected");if(m_onConnect)m_onConnect((int)c);}
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
}
