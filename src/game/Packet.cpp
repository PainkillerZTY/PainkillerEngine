#include "Packet.h"
#include <cstring>
namespace painkiller {
Packet::Packet(){}
void Packet::WriteU8(u8 v){m_data.push_back(v);}
void Packet::WriteU32(u32 v){m_data.push_back((v>>24)&0xFF);m_data.push_back((v>>16)&0xFF);m_data.push_back((v>>8)&0xFF);m_data.push_back(v&0xFF);}
void Packet::WriteF32(f32 v){u32*p=(u32*)&v;WriteU32(*p);}
void Packet::WriteString(const std::string& s){WriteU32((u32)s.size());for(char c:s)m_data.push_back((u8)c);}
void Packet::WriteBytes(const u8* data, u32 count){ for(u32 i=0;i<count;++i) m_data.push_back(data[i]); }
u8 Packet::ReadU8(){if(m_readPos>=m_data.size())return 0;return m_data[m_readPos++];}
u32 Packet::ReadU32(){return((u32)ReadU8()<<24)|((u32)ReadU8()<<16)|((u32)ReadU8()<<8)|ReadU8();}
f32 Packet::ReadF32(){u32 v=ReadU32();return *(f32*)&v;}
std::string Packet::ReadString(){u32 l=ReadU32();std::string s;for(u32 i=0;i<l;i++)s+=(char)ReadU8();return s;}
void Packet::ReadBytes(u8* out, u32 count){ for(u32 i=0;i<count;++i) out[i]=ReadU8(); }
void Packet::SetData(const u8* d,u32 s){m_data.assign(d,d+s);m_readPos=0;}
}
