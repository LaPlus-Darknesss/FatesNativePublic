#include "fates/chapter/chapter_run_state.hpp"
namespace fates::chapter {
bool IsMapLoadSkipped(std::uint8_t s){ return s==kChapterFlowMapLoadSkip; }
bool IsOpeningSkipped(std::uint8_t s){ return s==kChapterFlowOpeningSkip || s==kChapterFlowMapLoadSkip; }
bool IsSortieSkipped(bool versus,bool context,std::uint32_t chapter,std::uint32_t user,std::uint8_t state){
    const bool normal_path=context || ((chapter & 0x4u)!=0 && (user & 0x02000000u)==0);
    return versus || !normal_path || state==kChapterFlowMapLoadSkip;
}
std::uint32_t SanitizeUserFlagsBeforeChapterSave(std::uint32_t f){ return f & ~kChapterSaveTransientUserFlags; }
int ChapterSaveJumpState(std::uint32_t f){ if((f&0x01000000u)!=0) return 6; if((f&0x00000800u)!=0) return 5; return -1; }
std::uint8_t SelectGlobalLastRoute(bool has,std::uint32_t chapter_flags,std::uint8_t current){ return (!has || (chapter_flags&0x80u)!=0)?3u:current; }
std::uint32_t SanitizeGlobalFlagsForSerialize(std::uint32_t f){ return f & ~0x2u; }
std::uint8_t MapSettingEventType(bool resume,bool alt){ return resume?9u:(alt?0x23u:8u); }
int GetInfoCapabilityMode(std::uint32_t f){ if((f&kConfigInfoCapabilityMode2Bit)!=0) return 2; return (f&kConfigInfoCapabilityMode1Bit)!=0?1:0; }
std::uint32_t SetInfoCapabilityMode(std::uint32_t f,int mode){ f&=~(kConfigInfoCapabilityMode1Bit|kConfigInfoCapabilityMode2Bit); if(mode==1) f|=kConfigInfoCapabilityMode1Bit; else if(mode==2) f|=kConfigInfoCapabilityMode2Bit; return f; }
bool IsDistancePairAllowed(std::uint8_t d,std::uint8_t c){ if(d>2) return false; if(c==0) return true; if(c==1) return d==0||d==1; if(c==2) return d==0||d==2; if(c==3) return d==1||d==2; return false; }
std::uint8_t NormalizeDistance(std::uint8_t d,std::uint8_t c){ const std::uint8_t start=d%3; std::uint8_t x=start; for(int i=0;i<3;i++){ if(IsDistancePairAllowed(x,c)) return x; x=(x+1)%3; } return start; }
std::uint8_t NextDistance(std::uint8_t d,std::uint8_t c){ return NormalizeDistance((d+1)%3,c); }
bool ProfileRelianceDiffers(std::uint16_t a,std::uint16_t b){return a!=b;}
bool ProfileCastleAddressValid(std::uint32_t a){return a!=0;}
bool ProfileRouteOwned(std::uint16_t f,std::uint16_t mask){return (f&mask)!=0;}
std::array<std::uint8_t,9> SerializeUnitRecordV0(const UnitRecordV0&r){ return {0u,(std::uint8_t)r.a,(std::uint8_t)(r.a>>8),(std::uint8_t)r.b,(std::uint8_t)(r.b>>8),(std::uint8_t)r.c,(std::uint8_t)(r.c>>8),r.d,r.e}; }
UnitRecordV0 ClearUnitRecord(){return {};}
EndChapterUnitBits NormalizeEndChapterUnitBits(EndChapterUnitBits in){ in.flags8&=~0x40000000u; in.flagsC&=~0x7u; for(auto&x:in.status)x=(std::uint16_t)(x&0xff7fu); return in; }
}
