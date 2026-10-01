#include "fates/campaign/native_campaign_continuity.hpp"
#include <algorithm>

namespace fates::campaign::native {

std::uint32_t SetModeBits(std::uint32_t flags,int mode) {
    if (mode==0) return flags | 0xCu;
    if (mode==1) return (flags & ~0x8u) | 0x4u;
    return flags & ~0xCu;
}
int GetModeFromBits(std::uint32_t flags) {
    if ((flags & 0x8u)!=0) return 0;
    if ((flags & 0x4u)!=0) return 1;
    return 2;
}
bool IsRouteOrigin(std::uint8_t routeIndex,int query) {
    if (routeIndex==0) return query!=2;
    if (routeIndex==1) return query!=1;
    return routeIndex==2 || query==0;
}
bool IsCastleTime(bool hasVersusConfig,std::uint32_t flags,bool hasChapter,std::uint8_t chapterType) {
    if (hasVersusConfig || (flags&2u)!=0) return true;
    return hasChapter && (chapterType==4 || chapterType==5);
}
bool IsChapterRecord(bool hasVersusConfig,std::uint32_t flags,std::uint8_t chapterType) {
    return !hasVersusConfig && (flags&2u)==0 && chapterType!=3 && chapterType!=7 && chapterType!=4;
}
ChapterNameSource ResolveChapterNameSource(bool castleDefendSetting,bool hasChapter,std::uint8_t chapterType,bool hasVersusConfig,std::uint32_t flags) {
    if (castleDefendSetting || (hasChapter && chapterType==4)) return ChapterNameSource::CastleMessage;
    if (!hasChapter) return ChapterNameSource::None;
    if (hasVersusConfig || (flags&2u)!=0) return ChapterNameSource::TerrainName;
    return ChapterNameSource::TitleName;
}
ChapterPrefixSource ResolveChapterPrefixSource(bool castleDefendSetting,bool hasChapter,std::uint8_t chapterType,bool hasVersusConfig,std::uint32_t flags) {
    if (castleDefendSetting || (hasChapter && chapterType==4)) return ChapterPrefixSource::CastlePrefixMessage;
    if (!hasChapter) return ChapterPrefixSource::None;
    if (hasVersusConfig) return ChapterPrefixSource::VersusPrefixMessage;
    if ((flags&2u)!=0) return ChapterPrefixSource::TerrainPrefix;
    return ChapterPrefixSource::TitlePrefix;
}
WinRuleSource ResolveWinRuleSource(bool castleDefendSetting,bool hasVersusConfig,std::uint32_t flags,bool hasChapter,std::uint8_t chapterType) {
    if (castleDefendSetting) return WinRuleSource::Empty;
    if (hasVersusConfig || (flags&2u)!=0) return WinRuleSource::EncounterMessage;
    if (!hasChapter) return WinRuleSource::Empty;
    if (chapterType!=4) return WinRuleSource::ChapterRule;
    return (flags&0x20u)!=0 ? WinRuleSource::CastleOffenseMessage : WinRuleSource::CastleDefenseMessage;
}
int ClampNewTurnValue(int turn) { return std::clamp(turn,0,99); }
std::uint8_t UpdateEarliestTurnByte(std::uint8_t existing,std::uint32_t index,int requestedTurn) {
    if (index>=0x100u) return existing;
    const auto v=static_cast<std::uint8_t>(ClampNewTurnValue(requestedTurn));
    return (existing==0 || v<existing) ? v : existing;
}
int ResolveChapterSaveBeforeJump(std::uint32_t flags) {
    if ((flags&0x01000000u)!=0) return 6;
    if ((flags&0x00000800u)!=0) return 5;
    return -1;
}
SaveMenuPolicy ResolveSaveMenuPolicy(bool purchaseRouteFailed) { return {true,purchaseRouteFailed,4u}; }
bool ChapterRouteIncludes(std::uint8_t routeMask,RouteIndex route) { return (routeMask & (1u<<static_cast<unsigned>(route)))!=0; }
std::uint8_t SelectRouteNextChapter(std::uint8_t b,std::uint8_t c,std::uint8_t r,RouteIndex route) {
    switch(route){case RouteIndex::Birthright:return b;case RouteIndex::Conquest:return c;case RouteIndex::Revelation:return r;} return 0;
}
std::uint8_t SelectRouteRequirement(std::uint8_t b,std::uint8_t c,std::uint8_t r,RouteIndex route) { return SelectRouteNextChapter(b,c,r,route); }
bool IsMobEmpty(std::uint8_t kind,std::uint8_t state,bool hasPayload) {
    return !(state!=0 && kind!=0 && hasPayload && (kind!=1 || state==1));
}
int CountMobs(bool firstNonEmpty,bool secondNonEmpty) { return static_cast<int>(firstNonEmpty)+static_cast<int>(secondNonEmpty); }
int CurrentMobIndex(bool firstNonEmpty,std::uint8_t firstState,bool secondNonEmpty,std::uint8_t secondState) {
    if (firstNonEmpty && firstState==1) return 0;
    if (secondNonEmpty && secondState==1) return 1;
    return -1;
}
int TickMobLifetime(int remaining) { return remaining>0 ? remaining-1 : 0; }
MobCombatOutcome ResolveMobCombatRoll(int roll) {
    if (roll==0) return MobCombatOutcome::ResetFirst;
    if (roll==1) return MobCombatOutcome::ResetSecond;
    return MobCombatOutcome::ResetBoth;
}
bool IsMobDisposEnabled(std::uint32_t gameUserFlags,std::uint8_t spotState,int mobCount,int cooldown) {
    return (gameUserFlags&0x2000u)!=0 && spotState==2 && mobCount<2 && cooldown<1;
}
int SelectMobDisposSlot(bool slot0NonEmpty,bool slot1NonEmpty,int tieRoll) {
    if (slot0NonEmpty) return 1;
    if (slot1NonEmpty) return 0;
    return tieRoll==0 ? 1 : 0;
}
int ClampMobLevel(int level) { return std::clamp(level,0,200); }
int MobSpawnChanceForSortedIndex(int index) {
    if (index<4) return 20;
    if (index<6) return 15;
    if (index<8) return 5;
    if (index<12) return 3;
    return 1;
}
bool StopImmediateMobSpawnLoop(int successfulSpawns) { return successfulSpawns>=2; }

} // namespace fates::campaign::native
