#pragma once
#include "Types.h"
#include <vector>
#include <string>
namespace painkiller {
class Packet {
public:
    Packet();
    void WriteU8(u8 v);
    void WriteU32(u32 v);
    void WriteF32(f32 v);
    void WriteString(const std::string& s);
    u8 ReadU8();
    u32 ReadU32();
    f32 ReadF32();
    std::string ReadString();
    const std::vector<u8>& GetData() const { return m_data; }
    void SetData(const u8* data, u32 size);
    void Clear() { m_data.clear(); m_readPos = 0; }
private:
    std::vector<u8> m_data;
    u32 m_readPos = 0;
};
}
