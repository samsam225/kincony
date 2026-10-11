#pragma once
#include <cstdint>
#include <cstddef>
#include <array>
namespace avicole {
enum class Error { Ok, Timeout, Crc, Address, Function, Length, Exception, Gap, Overflow, Range, Warming, Unconfirmed, Stale };
inline const char* errorName(Error e) {
 const char* names[]={"OK","Timeout","CRC invalide","Adresse inattendue","Fonction inattendue","Longueur incorrecte","Exception Modbus","Silence RTU incorrect","Trame trop longue","Hors plage","Préchauffage","Non confirmé","Périmé"};
 return names[static_cast<unsigned>(e)];
}
inline uint16_t crc16(const uint8_t* p,size_t n) {
 uint16_t c=0xffff;while(n--){c^=*p++;for(int i=0;i<8;i++)c=(c&1)?(c>>1)^0xa001:c>>1;}return c;
}
inline uint16_t word(const uint8_t* p){return uint16_t(p[0])<<8|p[1];}
inline int32_t signedWord(uint16_t n){return n&0x8000?int32_t(n)-65536:n;}
inline std::array<uint8_t,8> request(uint8_t address,uint8_t function,uint16_t reg,uint16_t countOrValue){
 std::array<uint8_t,8> b={address,function,uint8_t(reg>>8),uint8_t(reg),uint8_t(countOrValue>>8),uint8_t(countOrValue),0,0};
 auto c=crc16(b.data(),6);b[6]=c&255;b[7]=c>>8;return b;
}
inline Error checkResponse(const uint8_t* p,size_t n,const std::array<uint8_t,8>& req,uint8_t& exception){
 exception=0;if(n<5)return Error::Length;
 if(crc16(p,n-2)!=uint16_t(p[n-2]|uint16_t(p[n-1])<<8))return Error::Crc;
 if(p[0]!=req[0])return Error::Address;
 if(p[1]==(req[1]|0x80)){if(n!=5)return Error::Length;exception=p[2];return Error::Exception;}
 if(p[1]!=req[1])return Error::Function;
 if(req[1]==6){if(n!=8)return Error::Length;for(int i=2;i<6;i++)if(p[i]!=req[i])return Error::Length;}
 else if(req[1]==3){auto bytes=word(req.data()+4)*2;if(n!=size_t(bytes+5)||p[2]!=bytes)return Error::Length;}
 else return Error::Function;
 return Error::Ok;
}
inline uint32_t t15us(uint32_t baud){return baud>19200?750:(16500000+baud-1)/baud;}
inline uint32_t t35us(uint32_t baud){return baud>19200?1750:(38500000+baud-1)/baud;}
inline float nhScale(int range){return range==50?0.1f:1.0f;}
inline uint64_t nhWarmupMs(int range){return range==500?172800000ULL:300000ULL;}
struct Decoded {float temperature=0,humidity=0,co2=0,nh3=0;};
inline Decoded decodeTs(const uint8_t* p){Decoded d;d.temperature=signedWord(word(p))/10.f;d.humidity=word(p+2)/10.f;d.co2=word(p+4);return d;}
inline Decoded decodeNh(const uint8_t* p,int range,bool combined){Decoded d;if(combined){d.humidity=word(p)/10.f;d.temperature=signedWord(word(p+2))/10.f;p+=4;}d.nh3=word(p)*nhScale(range);return d;}
}
