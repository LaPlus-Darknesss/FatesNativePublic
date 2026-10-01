#include "fates/campaign/persistence_world_state.hpp"
namespace fates::campaign {
int BackupFileCount(int t){switch(t){case 1:case 2:case 5:case 6:case 8:return 1;case 3:return 9;case 4:return 2;case 7:return 5;default:return 0;}}
const char16_t* BackupFixedFileName(int t){switch(t){case 1:return u"data:/Global";case 2:return u"data:/Exchange";case 5:return u"ext:/Temporary.bak";case 6:return u"ext:/Fate.bak";case 8:return u"data:/Rating";default:return nullptr;}}
std::size_t PackedFlagByteCount(std::uint32_t n){return (static_cast<std::size_t>(n)+7u)>>3u;}bool PackedFlagGet(const std::uint8_t*p,std::size_t n,std::uint32_t i){if(!p||i>=n)return false;return (p[i>>3u]&(std::uint8_t(1u)<<(i&7u)))!=0;}void PackedFlagSet(std::uint8_t*p,std::size_t n,std::uint32_t i){if(!p||i>=n)return;p[i>>3u]|=std::uint8_t(1u)<<(i&7u);}
Color8 ColorTableGet(const Color8*p,std::size_t n,std::size_t i){return(!p||i>=n)?Color8{}:p[i];}std::size_t ColorTableByteCount(std::uint32_t n){return static_cast<std::size_t>(n)*4u;}
void ResetSpotMob(SpotMobPortable&m){m.kind=0;m.active=0;m.words.fill(0);m.tail=-1;}bool IsSpotMobEmpty(const SpotMobPortable&m){if(m.active==0)return true;if(m.kind==0)return true;return false;}unsigned CountSpotMobs(const SpotMobPortable&a,const SpotMobPortable&b){return (IsSpotMobEmpty(a)?0u:1u)+(IsSpotMobEmpty(b)?0u:1u);}
}
