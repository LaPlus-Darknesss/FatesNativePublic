#include "fates/chapter/chapter_run_state_depth.hpp"
namespace fates::chapter{
std::size_t PackedFlagBytes(std::uint32_t n){return (static_cast<std::size_t>(n)+7u)>>3;}
std::size_t Color8TableBytes(std::uint32_t n){return static_cast<std::size_t>(n)*4u;}
bool TestPackedFlag(const std::uint8_t*p,std::size_t n,std::uint32_t bit){auto i=static_cast<std::size_t>(bit>>3);return p&&i<n&&(p[i]&(1u<<(bit&7u)))!=0;}
void SetPackedFlag(std::uint8_t*p,std::size_t n,std::uint32_t bit){auto i=static_cast<std::size_t>(bit>>3);if(p&&i<n)p[i]=static_cast<std::uint8_t>(p[i]|(1u<<(bit&7u)));}
int VersusTimeLimitSeconds(int c){return c==0?180:c==1?300:c==2?600:0;}
bool LinkFailureFromCode(int c){return c!=0;}
bool MatchClientFromSessionMaster(bool master){return !master;}
bool MatchMasterFromSlots(std::uint8_t local,std::uint8_t master){return local<12u&&local==master;}
std::size_t UnitEditInitialDeserializeBytes(std::uint8_t version){return version==0?0x0Eu:0x1Au;}
}
