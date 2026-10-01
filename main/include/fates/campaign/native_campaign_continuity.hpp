#pragma once
#include <cstdint>

namespace fates::campaign::native {

constexpr int kGameUserDataBytes = 0x54;
constexpr int kGameUserChapterRecordCount = 0x40;
constexpr int kGameUserChapterRecordBytes = 0x10;
constexpr int kGameUserFlagCapacity = 0x80;
constexpr int kGameUserVariableCapacity = 0x30;
constexpr int kGameUserNoNameFlagCapacity = 0x30;
constexpr std::uint8_t kGameUserSerializeVersion = 6;
constexpr std::uint8_t kGameUserRandomSeedFormatVersion = 4;
constexpr std::uint32_t kChapterSaveBeforeClearMask = 0x07000EE2u;

enum class RouteIndex : std::uint8_t { Birthright=0, Conquest=1, Revelation=2 };
enum class ChapterType : std::uint8_t { Story=0, Paralogue=1, Unknown2=2, DragonsGate=3, Unknown4=4, Invasion=5, Ending=6, HeroBattles=7 };

std::uint32_t SetModeBits(std::uint32_t flags, int mode);
int GetModeFromBits(std::uint32_t flags);
bool IsRouteOrigin(std::uint8_t routeIndex, int query);
bool IsCastleTime(bool hasVersusConfig, std::uint32_t flags, bool hasChapter, std::uint8_t chapterType);
bool IsChapterRecord(bool hasVersusConfig, std::uint32_t flags, std::uint8_t chapterType);

enum class ChapterNameSource : std::uint8_t { None, CastleMessage, TerrainName, TitleName };
enum class ChapterPrefixSource : std::uint8_t { None, CastlePrefixMessage, VersusPrefixMessage, TerrainPrefix, TitlePrefix };
enum class WinRuleSource : std::uint8_t { Empty, EncounterMessage, CastleOffenseMessage, CastleDefenseMessage, ChapterRule };
ChapterNameSource ResolveChapterNameSource(bool castleDefendSetting, bool hasChapter, std::uint8_t chapterType, bool hasVersusConfig, std::uint32_t flags);
ChapterPrefixSource ResolveChapterPrefixSource(bool castleDefendSetting, bool hasChapter, std::uint8_t chapterType, bool hasVersusConfig, std::uint32_t flags);
WinRuleSource ResolveWinRuleSource(bool castleDefendSetting, bool hasVersusConfig, std::uint32_t flags, bool hasChapter, std::uint8_t chapterType);

int ClampNewTurnValue(int turn);
std::uint8_t UpdateEarliestTurnByte(std::uint8_t existing, std::uint32_t index, int requestedTurn);
int ResolveChapterSaveBeforeJump(std::uint32_t flags);
struct SaveMenuPolicy { bool byte138{}; bool byte139{}; std::uint32_t flag50Or{}; };
SaveMenuPolicy ResolveSaveMenuPolicy(bool purchaseRouteFailed);

bool ChapterRouteIncludes(std::uint8_t routeMask, RouteIndex route);
std::uint8_t SelectRouteNextChapter(std::uint8_t birthright, std::uint8_t conquest, std::uint8_t revelation, RouteIndex route);
std::uint8_t SelectRouteRequirement(std::uint8_t birthright, std::uint8_t conquest, std::uint8_t revelation, RouteIndex route);

bool IsMobEmpty(std::uint8_t kind, std::uint8_t state, bool hasPayload);
int CountMobs(bool firstNonEmpty, bool secondNonEmpty);
int CurrentMobIndex(bool firstNonEmpty, std::uint8_t firstState, bool secondNonEmpty, std::uint8_t secondState);
int TickMobLifetime(int remaining);
enum class MobCombatOutcome : std::uint8_t { ResetFirst, ResetSecond, ResetBoth };
MobCombatOutcome ResolveMobCombatRoll(int roll);
bool IsMobDisposEnabled(std::uint32_t gameUserFlags, std::uint8_t spotState, int mobCount, int cooldown);
int SelectMobDisposSlot(bool slot0NonEmpty, bool slot1NonEmpty, int tieRoll);
int ClampMobLevel(int level);
int MobSpawnChanceForSortedIndex(int index);
bool StopImmediateMobSpawnLoop(int successfulSpawns);

} // namespace fates::campaign::native
