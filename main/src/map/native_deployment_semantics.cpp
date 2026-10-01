#include "fates/map/native_deployment_semantics.hpp"
#include <algorithm>

namespace fates::map::native {

int ManhattanDistance(int x0,int y0,int x1,int y1) {
    const int dx=x0-x1; const int dy=y0-y1;
    return (dx<0?-dx:dx)+(dy<0?-dy:dy);
}
bool IsDistanceInInclusiveBand(int distance,int minimum,int maximum) {
    return minimum <= distance && distance <= maximum;
}
int ResolveMoveImageCell(bool terrainBlocksMovement,std::int8_t storedValue) {
    return terrainBlocksMovement ? -1 : static_cast<int>(storedValue);
}
bool IsFillAllowed(const FillCellInput& in,unsigned flags) {
    if (in.terrainBlocksMovement || in.moveImageValue < 0) return false;
    if (!in.occupied || in.moveImageValue == 0) return true;
    if ((flags & kDeployFlagBlockOccupied) != 0) return false;
    if ((flags & kDeployFlagRespectMovementBlock) != 0 &&
        (in.occupantMovementProhibited || in.occupantState100)) return false;
    if ((flags & kDeployFlagRequireAlliedOccupant) != 0 && !in.occupantAllied) return false;
    return true;
}
void MergeRetailItemRange(DistanceMask64& out,unsigned inner,unsigned outer) {
    if (out.maxOuter < static_cast<int>(outer)) out.maxOuter=static_cast<int>(outer);
    // Retail treats 0xFF as an open/special outer range: it records maxOuter but does not
    // synthesize ordinary distance bits for that item.
    if (outer==0xFFu || inner>outer) return;
    for (unsigned d=inner; d<=outer; ++d) if (d<64u) out.bits |= (std::uint64_t{1}<<d);
}
CannonRange ResolveCannonRange(int rangeInner,int rangeOuter,int area) {
    return {std::max(0,rangeInner-area), rangeOuter+area};
}
UnitFillRoute ResolveUnitFillRoute(int attackMaxOuter,int rodMaxOuter) {
    if (attackMaxOuter==0 && rodMaxOuter==0) return UnitFillRoute::None;
    if (attackMaxOuter!=0 && rodMaxOuter==0) return UnitFillRoute::AttackOnly;
    if (attackMaxOuter==0 && rodMaxOuter!=0) return UnitFillRoute::RodOnly;
    return UnitFillRoute::AttackAndRod;
}
int ResolveAiMovePower(unsigned unitAiFlags,int requestedMovePower) {
    return (unitAiFlags & 0x100u) != 0 ? 0 : requestedMovePower;
}
bool ShouldApplyAiRectangleLimit(std::uint8_t aiState,unsigned flags) {
    return aiState==4 && (flags & kDeployFlagAiRectangleLimit)!=0;
}
bool AiRectangleAllowsCell(int x,int y,int left,int top,int width,int height,int originX,int originY) {
    if (x==originX && y==originY) return true;
    return x>=left && y>=top && x<left+width && y<top+height;
}
bool DeserializeShouldLoad(std::string_view disposName) { return !disposName.empty(); }
DisposLoadChoice ResolveDisposLoadChoice(bool routeSpecificExists,bool commonExists) {
    if (routeSpecificExists) return DisposLoadChoice::RouteSpecific;
    if (commonExists) return DisposLoadChoice::CommonFallback;
    return DisposLoadChoice::Missing;
}
bool DisposModeFlagAllows(std::uint8_t modeByte,unsigned spawnFlags) {
    if (modeByte==0) return (spawnFlags & 0x100u)!=0;
    if (modeByte==1) return (spawnFlags & 0x200u)!=0;
    if (modeByte==2) return (spawnFlags & 0x400u)!=0;
    return true;
}
std::uint32_t DisposDifficultySpawnMask(std::uint8_t difficulty) noexcept {
    if (difficulty==0) return 0x100u;
    if (difficulty==1) return 0x200u;
    if (difficulty==2) return 0x400u;
    return 0u;
}
bool DisposDifficultyAllows(std::uint8_t difficulty,unsigned spawnFlags) noexcept {
    const auto mask=DisposDifficultySpawnMask(difficulty);
    return mask!=0 && (spawnFlags & mask)!=0;
}
bool DisposIsPairLead(unsigned spawnFlags) noexcept { return (spawnFlags & 0x10000u)!=0; }
bool DisposIsPairPartner(unsigned spawnFlags) noexcept { return (spawnFlags & 0x20000u)!=0; }
bool ShouldAttemptCreateSortie(std::uint8_t gameMode,std::uint8_t state,bool residentMode,
                               bool personResident,std::uint8_t team,unsigned spawnFlags) {
    if (gameMode==4 || state==3) return false;
    if (residentMode && personResident) return true;
    if (team==0 && (spawnFlags & 0x1000u)==0) return false;
    return true;
}
std::uint8_t ResolveSpotForce(std::uint8_t defaultForce,unsigned calculateFlags,
                              std::uint8_t spotForce40,std::uint8_t spotForce80,int mobCount) {
    std::uint8_t force=defaultForce;
    if ((calculateFlags & 0x40u)!=0) force=spotForce40;
    if ((calculateFlags & 0x80u)!=0) force=spotForce80;
    if (mobCount==1) force=1;
    return force;
}
std::uint32_t ResolveCreatedUnitFlags(std::uint32_t currentFlags,unsigned spawnFlags,
                                     unsigned calculateFlags,bool uniquePersonMatched) {
    std::uint32_t out=currentFlags | 0x8000u;
    if ((spawnFlags & 0xCu)!=0 && uniquePersonMatched) {
        out |= 0x200u;
        if ((spawnFlags & 4u)!=0) out |= 0x1000u;
    }
    if ((spawnFlags & 0x80u)!=0) out |= 0x2000u;
    if ((calculateFlags & 0x2000u)!=0) out |= 1u;
    return out;
}
bool DisposCalculateEarlyReject(std::uint8_t state,unsigned spawnFlags) {
    return state!=0 || (spawnFlags & 0x20000u)!=0;
}
bool ForceCapacityRejectsDispos(int forceCount) { return forceCount>0x31; }
int SpotShuffleDrawCount(int recordCount) { return recordCount>1 ? recordCount-1 : 0; }
int SpotShuffleIndexFromDraw(int draw,int recordCount) {
    if (recordCount<=1 || draw<0 || draw>=recordCount-1) return -1;
    return draw+1;
}
bool IsSortiePositionCandidate(std::uint8_t state,unsigned spawnFlags,std::uint8_t team,
                               bool coordinateInMap,bool terrainPassable) {
    if (state==3 || team!=0) return false;
    if ((spawnFlags & 0x7Cu)==0 || (spawnFlags & 0x44u)!=0) return false;
    return coordinateInMap && terrainPassable;
}

} // namespace fates::map::native
