#include "fates/map/native_terrain_semantics.hpp"

#include <algorithm>

namespace fates::map::native {
namespace {
bool InRect(const Rect& r, int x, int y) noexcept {
    return r.width > 0 && r.height > 0 && x >= r.x && y >= r.y &&
           x <= r.x + r.width - 1 && y <= r.y + r.height - 1;
}
// ARM arithmetic shift rounds negative odd halves downward.
int HalfFloor(int value) noexcept {return value/2-((value<0&&value%2)?1:0);}
int Clamp(int value, int lo, int hi) noexcept { return std::min(hi, std::max(lo, value)); }
} // namespace

std::size_t TerrainCostStride(std::uint32_t width) noexcept {
    return (static_cast<std::size_t>(width) + 3u) & ~std::size_t{3u};
}
std::size_t TerrainTileOffset(std::uint8_t terrain_id) noexcept {
    return static_cast<std::size_t>(terrain_id) * kTerrainTileRecordBytes;
}
void HideTerrainInfo(TerrainInfoState& state) noexcept {
    state.x=-1; state.y=-1; state.has_terrain=false; state.terrain_id=0;
}
void ShowTerrainInfo(TerrainInfoState& state, int x, int y, std::uint8_t terrain_id) noexcept {
    state.x=x; state.y=y; state.has_terrain=true; state.terrain_id=terrain_id;
}
bool IsCastleUnitDispos(const TerrainTileResolved& t, int knight_movement_cost) noexcept {
    return t.move_cost_index != 0 && (t.retail_flags & 0x8u) == 0 && knight_movement_cost > 0;
}

bool ShouldSerializeTerrainDelta(const TerrainTileResolved& base, std::uint8_t current_id) noexcept {
    return base.change_id_1==0 && base.change_id_2==0 && current_id!=base.id;
}
bool CanApplyTerrainDelta(const TerrainTileResolved& base, int delta_id) noexcept {
    return delta_id >= 0 && base.change_id_1==0 && base.change_id_2==0;
}
bool IsShowDeploy(std::uint8_t type, std::uint8_t flags) noexcept {
    return (flags&1u)!=0 && (type==0x0Bu || (type>=0x19u && type<=0x1Cu));
}
bool IsCannon(std::uint8_t type) noexcept { return type>=0x19u && type<=0x1Bu; }
std::optional<int> CannonWeaponExpKind(std::uint8_t type, bool kind5_blocked) noexcept {
    if(type==0x19u) return 3;
    if(type==0x1Au) return 4;
    if(type==0x1Bu && !kind5_blocked) return 5;
    return std::nullopt;
}
bool CanUseCannon(std::uint8_t type, int e3, int e4, int e5, bool kind5_blocked) noexcept {
    const auto kind=CannonWeaponExpKind(type,kind5_blocked);
    if(!kind) return false;
    return (*kind==3?e3:(*kind==4?e4:e5))>=1;
}
bool IsDestroyTargetCastleOffense(std::uint8_t type) noexcept { return type>=0x0Fu && type<=0x16u; }
std::pair<int,int> CannonAccess(const Rect& r, std::uint8_t d) noexcept {
    const int cx=r.x+HalfFloor(r.width-1), cy=r.y+HalfFloor(r.height-1);
    switch(d){
        case 1: return {r.x-1,cy};
        case 2: return {r.x+r.width,cy};
        case 4: return {cx,r.y+r.height};
        case 8: return {cx,r.y-1};
        default:return {cx,cy};
    }
}
std::pair<int,int> CannonUnitDirection(const Rect& r, std::pair<int,int> access) noexcept {
    if(access.first<0) return {0,0};
    const int cx=r.x+HalfFloor(r.width-1), cy=r.y+HalfFloor(r.height-1);
    return {cx-access.first,cy-access.second};
}
bool IsTrickAccess(std::uint8_t type,const Rect& r,int x,int y,bool has_change1,
                   std::pair<int,int> cannon_access) noexcept {
    if(IsCannon(type)) return cannon_access==std::pair<int,int>{x,y};
    switch(type){
      case 1:case 2:case 3:case 4:case 5:case 6:case 7:case 8:case 0x0B:case 0x0D:case 0x0E:case 0x17:case 0x18:
        return InRect(r,x,y);
      case 9:case 10:{
        Rect e{r.x-1,r.y-1,r.width+2,r.height+2};
        if(!InRect(e,x,y)) return false;
        const bool edgeX=(x==e.x || x==e.x+e.width-1), edgeY=(y==e.y || y==e.y+e.height-1);
        return !(edgeX&&edgeY);
      }
      case 0x1D: return InRect(r,x,y)&&has_change1;
      default:return false;
    }
}
bool IsBreakable(std::uint8_t breakable_value,bool flag2) noexcept { return breakable_value>0 && flag2; }
bool ShouldMarkDeploy(std::uint32_t f,std::uint8_t tf) noexcept {
    return ((f&0x400u)!=0&&(tf&0x1u)!=0)||((f&0x800u)!=0&&(tf&0x2u)!=0)||((f&0x1000u)!=0&&(tf&0x4u)!=0);
}
DeployPropagation DeployPropagationForTerrain(std::uint32_t f) noexcept { return {(f&0x2000u)!=0,(f&0x4000u)!=0}; }

bool CanSetTerrainTid(bool map_exists,bool inside,const TerrainTileResolved* cur,const TerrainTileResolved* target) noexcept {
    return map_exists&&inside&&cur&&target&&cur->change_id_1==0&&cur->change_id_2==0&&target->change_id_1==0&&target->change_id_2==0;
}
bool TerrainIsTid(bool map_exists,bool inside,bool target_exists,std::uint8_t current,std::uint8_t target) noexcept {
    return map_exists&&inside&&target_exists&&current==target;
}
bool TerrainHasFlag(bool map_exists,bool inside,std::uint32_t flags,std::uint32_t mask) noexcept {
    return map_exists&&inside&&(flags&mask)!=0;
}
bool ShouldRemoveObstacle(std::uint32_t flags) noexcept { return (flags&0x00800000u)!=0; }

bool CanDragonVein(std::uint64_t private_mask,std::uint64_t combined,bool unit_block,bool download) noexcept {
    if((private_mask&combined)==0) return false;
    return !unit_block||download;
}
int CalculateDragonVeinScore(const DragonVeinScoreInput& in) noexcept {
    if(in.situation_blocked) return 0;
    if(in.chapter_type==4){
        if(in.user_flag_0x20) {
            const auto signed_value=static_cast<std::int32_t>(in.stored_value_bits);
            return Clamp((signed_value-in.capability_deduction)/10+100,10,300);
        }
        const auto unsigned_div10=in.stored_value_bits/10u;
        return Clamp(static_cast<int>((unsigned_div10+100u)/2u)-in.trick_count*10,10,100);
    }
    if(in.chapter_type==5) return Clamp(300-in.trick_count*20,10,300);
    return 100;
}

} // namespace fates::map::native
