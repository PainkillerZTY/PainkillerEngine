#pragma once
#include "Types.h"
#include <vector>
#include <string>
namespace painkiller {

// ============================================================
// Packet Type IDs
// ============================================================
namespace PacketType {
    constexpr u8 PlayerPosition = 0x01; // Client↔Server: position (client→server unicast, server→clients broadcast w/playerId)
    constexpr u8 BlockChange    = 0x02; // Server→Clients: block modification
    constexpr u8 ChunkData      = 0x03; // Server→Client: chunk block data
    constexpr u8 PlayerJoin     = 0x04; // Server→Clients: new player
    constexpr u8 PlayerLeave    = 0x05; // Server→Clients: player disconnected
}

class Packet {
public:
    Packet();
    void WriteU8(u8 v);
    void WriteU32(u32 v);
    void WriteF32(f32 v);
    void WriteString(const std::string& s);
    void WriteBytes(const u8* data, u32 count);
    u8 ReadU8();
    u32 ReadU32();
    f32 ReadF32();
    std::string ReadString();
    void ReadBytes(u8* out, u32 count);
    const std::vector<u8>& GetData() const { return m_data; }
    void SetData(const u8* data, u32 size);
    void Clear() { m_data.clear(); m_readPos = 0; }
private:
    std::vector<u8> m_data;
    u32 m_readPos = 0;
};
}
