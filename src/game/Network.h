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
private:
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
private:
    SOCKET m_socket = INVALID_SOCKET;
    bool m_connected = false;
};
}
