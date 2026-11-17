#include "capture/checksum.hpp"
#include <cstdint>
#include <iomanip>
#include <sstream>
#include <stdexcept>
namespace capture {
std::string save_checksum(std::string_view s) {
 std::uint32_t h=2166136261u;
 auto add=[&](std::uint32_t unit){h^=unit;h*=16777619u;};
 for(std::size_t i=0;i<s.size();) {
  const auto first=static_cast<unsigned char>(s[i++]);std::uint32_t cp{};int extra{};
  if(first<128)cp=first;
  else if((first&0xe0)==0xc0){cp=first&31;extra=1;}
  else if((first&0xf0)==0xe0){cp=first&15;extra=2;}
  else if((first&0xf8)==0xf0){cp=first&7;extra=3;}
  else throw std::invalid_argument("invalid UTF-8");
  for(int n=0;n<extra;++n) {
   if(i>=s.size())throw std::invalid_argument("truncated UTF-8");
   const auto b=static_cast<unsigned char>(s[i++]);if((b&0xc0)!=0x80)throw std::invalid_argument("invalid UTF-8 continuation");cp=(cp<<6)|(b&63);
  }
  if((extra==1&&cp<128)||(extra==2&&cp<2048)||(extra==3&&cp<65536)||cp>0x10ffff||(cp>=0xd800&&cp<=0xdfff))throw std::invalid_argument("invalid UTF-8 scalar");
  if(cp>65535){cp-=65536;add(0xd800+(cp>>10));add(0xdc00+(cp&1023));}else add(cp);
 }
 std::ostringstream out;out<<std::hex<<std::setfill('0')<<std::setw(8)<<h;return out.str();
}
}
