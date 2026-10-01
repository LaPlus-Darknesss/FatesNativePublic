#pragma once
#include <cstdint>
#include <string_view>

namespace fates::map::native {

constexpr unsigned kDeployFlagBlockOccupied = 0x00000004u;
constexpr unsigned kDeployFlagRespectMovementBlock = 0x00000010u;
constexpr unsigned kDeployFlagRequireAlliedOccupant = 0x00004000u;
constexpr unsigned kDeployFlagAiRectangleLimit = 0x00040000u;
constexpr unsigned kDeployFlagIncludePairPartnerRange = 0x00008000u;

int ManhattanDistance(int x0, int y0, int x1, int y1);
bool IsDistanceInInclusiveBand(int distance, int minimum, int maximum);
int ResolveMoveImageCell(bool terrainBlocksMovement, std::int8_t storedValue);

struct FillCellInput {
    bool terrainBlocksMovement{};
    int moveImageValue{};
    bool occupied{};
    bool occupantMovementProhibited{};
    bool occupantState100{};
    bool occupantAllied{};
};
bool IsFillAllowed(const FillCellInput& in, unsigned flags);

struct DistanceMask64 { std::uint64_t bits{}; int maxOuter{}; };
void MergeRetailItemRange(DistanceMask64& out, unsigned inner, unsigned outer);
struct CannonRange { int inner{}; int outer{}; };
CannonRange ResolveCannonRange(int rangeInner, int rangeOuter, int area);

enum class UnitFillRoute : std::uint8_t { None, AttackOnly, RodOnly, AttackAndRod };
UnitFillRoute ResolveUnitFillRoute(int attackMaxOuter, int rodMaxOuter);
int ResolveAiMovePower(unsigned unitAiFlags, int requestedMovePower);
bool ShouldApplyAiRectangleLimit(std::uint8_t aiState, unsigned flags);
bool AiRectangleAllowsCell(int x, int y, int left, int top, int width, int height, int originX, int originY);

bool DeserializeShouldLoad(std::string_view disposName);
enum class DisposLoadChoice : std::uint8_t { Missing, RouteSpecific, CommonFallback };
DisposLoadChoice ResolveDisposLoadChoice(bool routeSpecificExists, bool commonExists);
bool DisposModeFlagAllows(std::uint8_t modeByte, unsigned spawnFlags);
// Retail map::Dispos::Data::IsEnable reads GameUserData difficulty 0/1/2 and
// requires spawn flag 0x100/0x200/0x400 respectively.
std::uint32_t DisposDifficultySpawnMask(std::uint8_t difficulty) noexcept;
bool DisposDifficultyAllows(std::uint8_t difficulty, unsigned spawnFlags) noexcept;
// map::Dispos::Calculate treats 0x10000 as a paired lead and consumes the
// immediately following 0x20000 record through Unit::DoubleOn rather than as
// an independent map actor.
bool DisposIsPairLead(unsigned spawnFlags) noexcept;
bool DisposIsPairPartner(unsigned spawnFlags) noexcept;
bool ShouldAttemptCreateSortie(std::uint8_t gameMode, std::uint8_t state, bool residentMode,
                               bool personResident, std::uint8_t team, unsigned spawnFlags);
std::uint8_t ResolveSpotForce(std::uint8_t defaultForce, unsigned calculateFlags,
                              std::uint8_t spotForce40, std::uint8_t spotForce80, int mobCount);
std::uint32_t ResolveCreatedUnitFlags(std::uint32_t currentFlags, unsigned spawnFlags,
                                      unsigned calculateFlags, bool uniquePersonMatched);
bool DisposCalculateEarlyReject(std::uint8_t state, unsigned spawnFlags);
bool ForceCapacityRejectsDispos(int forceCount);
int SpotShuffleDrawCount(int recordCount);
int SpotShuffleIndexFromDraw(int draw, int recordCount);
bool IsSortiePositionCandidate(std::uint8_t state, unsigned spawnFlags, std::uint8_t team,
                               bool coordinateInMap, bool terrainPassable);

} // namespace fates::map::native
