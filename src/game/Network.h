#pragma once
#include "Types.h"
#include "Packet.h"
#include <functional>
#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
namespace painkiller {
class NetworkServer {
public:
    NetworkServer();
    ~NetworkServer();
    bool Start(u16 port);
    void Stop();
    void Update();
    bool IsRunning() const { return m_running; }
    void SetOnClientConnect(std::function<void(int)> cb) { m_onConnect = cb; }
    void SetOnClientDisconnect(std::function<void(int)> cb) { m_onDisconnect = cb; }
    void Broadcast(const u8* data, u32 size);
    void SendTo(int clientId, const u8* data, u32 size);
    int GetClientCount() const { return (int)m_clients.size(); }
    void SetOnPacketReceived(std::function<void(int, const u8*, u32)> cb) { m_onPacket = cb; }
    struct ClientInfo { SOCKET socket; int id; };
    std::vector<ClientInfo> m_clients;
    int m_nextId = 1;
private:
    std::function<void(int, const u8*, u32)> m_onPacket;
    SOCKET m_listenSocket = INVALID_SOCKET;
    bool m_running = false;
    std::function<void(int)> m_onConnect;
    std::function<void(int)> m_onDisconnect;
};
class NetworkClient {
public:
    NetworkClient();
    ~NetworkClient();
    bool Connect(const char* host, u16 port);
    void Disconnect();
    bool IsConnected() const { return m_connected; }
    void Update();
    void Send(const u8* data, u32 size);
    void SetOnPacketReceived(std::function<void(const u8*, u32)> cb) { m_onPacket = cb; }
    void SetOnDisconnect(std::function<void()> cb) { m_onDisconnect = cb; }
private:
    SOCKET m_socket = INVALID_SOCKET;
    bool m_connected = false;
    std::vector<u8> m_recvBuf;
    u32 m_recvState = 0; // 0 = waiting for 4-byte len, 1 = waiting for payload
    u32 m_pendingLen = 0;
    std::vector<u8> m_pendingData;
    std::function<void(const u8*, u32)> m_onPacket;
    std::function<void()> m_onDisconnect;
};
}
