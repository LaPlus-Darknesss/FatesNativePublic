#include "fates/runtime/native_item_inventory.hpp"
#include "fates/ai/native_ai_position_movement.hpp"
#include "fates/ai/native_ai_movement_fields.hpp"
#include "fates/ai/native_ai_action.hpp"
#include "fates/runtime/native_player_action.hpp"
#include <algorithm>
#include <array>
#include <cstdlib>
#include <memory>

namespace fates::ai::native {
namespace rn=fates::runtime::native;
int OrdinaryTerrainScore(std::int8_t defense,std::int8_t avoid,std::int8_t healing) noexcept {
    // 004CB228..004CB250: SMULL 0x66666667; ASR 1/2 with sign
    // correction implements signed division by 5/10, then USAT #6.
    return std::clamp(5+int(avoid)+int(defense)/5+int(healing)/10,0,63);
}
std::uint32_t OrdinaryMoveToScore(int actual,int reverse,int terrain,int allies,
                                 int gx,int gy,int x,int y) noexcept {
    const std::int64_t weighted=(std::int64_t(terrain)+(100-std::int64_t(reverse))*16)*8;
    const auto diagonal=31-std::abs(std::abs(std::int64_t(gx)-x)-std::abs(std::int64_t(gy)-y));
    return static_cast<std::uint32_t>(diagonal+((100-std::int64_t(actual))+(allies+weighted)*128)*32);
}
namespace {
bool EmptyArgs(const std::array<std::int16_t,4>& args) {return std::all_of(args.begin(),args.end(),[](auto x){return x==-1;});}
template<class T> bool Zero(const T& a) {return std::all_of(a.begin(),a.end(),[](auto x){return x==0;});}
bool Live(const rn::UnitState& u) {return u.occupied && u.has_position && !u.defeated && fates::runtime::native::IsTacticalForce(u.force_type) && u.pair.role!=rn::PairRole::Partner;}
bool Profile(const rn::NativeRuntime& r,const rn::UnitState& u) {
    const auto* person=r.definitions.FindPerson(u.person_id);const auto* job=r.definitions.FindJob(u.job_id);
    if(!person || !job || !r.definitions.terrain_map() || job->movement==0 || job->movement>100)return false;
    // No unrepresented private movement flags, class flight, movement/equipment
    // skills, pair/special order behavior, or movement-limit masks are assumed.
    if(!Zero(person->bitflags) || !Zero(job->bitflags) || !Zero(u.equipped_skill_ids) ||
       !Zero(person->personal_skills) || !Zero(job->special_flags) || u.pair.bound || u.pair.role!=rn::PairRole::None)return false;
    if((u.ai.policy_flags&~0x0041E6FFu)!=0 || u.ai.move_limit_mode!=0 ||
       u.ai.move_limit_x1!=255 || u.ai.move_limit_y1!=255 || u.ai.move_limit_x2!=255 || u.ai.move_limit_y2!=255)return false;
    // Current native force model has no general Force::IsAllied matrix.
    if(u.force_type>1)return false;
    for(const auto& other:r.game.units)if(Live(other) && other.force_type>1)return false;
    const auto inventory=rn::ReadCalculationInventory(u);if(!inventory)return false;
    if(u.equipped_item_id && std::none_of(inventory->begin(),inventory->end(),[&](auto item){return item.item_id==u.equipped_item_id;}))return false;
    const auto& map=*r.definitions.terrain_map();
    for(const auto& t:map.terrain_types) if((t.flags_0x18&1u) && t.change_id_2 &&
        std::none_of(map.terrain_types.begin(),map.terrain_types.end(),[&](const auto& v){return v.id==t.change_id_2;}))return false;
    return true;
}
}
detail::OrdinaryAiField detail::BuildOrdinaryAiField(const rn::NativeRuntime& r,std::uint16_t slot,int x,int y,int budget,rn::MovementFieldOptions options) {
    OrdinaryAiField grid;
    for(const auto& cell:rn::EnumerateUnitMovementField(r,slot,std::int16_t(x),std::int16_t(y),budget,options)) {
        const auto* terrain=r.definitions.TerrainAt(cell.x,cell.y);
        if(!terrain || (terrain->flags_0x18&8u)) continue; // MoveImage::Get
        const auto i=std::size_t(cell.y*32+cell.x);grid.cost[i]=cell.cost;grid.stop[i]=cell.occupiable;
    }
    return grid;
}
int detail::OrdinaryAdjacentAllyCount(const rn::NativeRuntime& r,std::uint16_t slot,int x,int y) {
    int count=0;const auto force=r.game.units[slot].force_type;
    constexpr int dx[]{1,-1,0,0},dy[]{0,0,1,-1};
    for(int k=0;k<4;++k) for(std::uint16_t i=0;i<r.game.units.size();++i) {
        const auto& u=r.game.units[i];if(i==slot || !Live(u) || u.x!=x+dx[k] || u.y!=y+dy[k])continue;
        if(u.force_type==force && (u.action_committed || (u.flags&0xC5u) || (u.ai.policy_flags&0x400000u))) ++count;
        break;
    }
    return count;
}

AiPositionMoveResult ExecuteConfiguredPositionMovement(rn::NativeRuntime& r,std::uint16_t slot,std::uint16_t turn) {
    AiPositionMoveResult out{};out.unit_slot=slot;
    if(slot>=r.game.units.size())return out;
    const auto& u=r.game.units[slot];
    out.start_x=out.end_x=u.x;out.start_y=out.end_y=u.y;
    if(!Live(u) || u.action_committed)return out;
    if(!u.ai.configured || !u.ai.runtime_tuning_bound || u.ai.mission_id!=kMissionNull ||
       u.ai.attack_id!=kAttackNull || u.ai.movement_id!=kMovementPosition ||
       !EmptyArgs(u.ai.mission_args) || !EmptyArgs(u.ai.attack_args) ||
       u.ai.movement_args[2]!=-1 || u.ai.movement_args[3]!=-1) {
        out.status=AiPositionMoveStatus::UnsupportedDescriptor;return out;
    }
    if(u.ai.action_id==kActionEverytime) {
        if(!EmptyArgs(u.ai.action_args)) {out.status=AiPositionMoveStatus::UnsupportedDescriptor;return out;}
    } else if(u.ai.action_id==kActionTurn && HasSingleTurnArgumentShape(u)) {
        if(turn<unsigned(u.ai.action_args[0])) {out.status=AiPositionMoveStatus::Inactive;return out;}
    } else {out.status=AiPositionMoveStatus::UnsupportedDescriptor;return out;}
    if(!Profile(r,u)) {out.status=AiPositionMoveStatus::UnsupportedProfile;return out;}
    const auto& map=*r.definitions.terrain_map();
    const int gx=u.ai.movement_args[0],gy=u.ai.movement_args[1];out.goal_x=std::int16_t(gx);out.goal_y=std::int16_t(gy);
    // ActionMovePosition reads two signed bytes. Reject out-of-domain arguments
    // rather than allowing the wider host representation to silently wrap.
    if(gx<int(map.min_x)||gy<int(map.min_y)||gx>=int(map.max_x)||gy>=int(map.max_y)||gx>127||gy>127) {
        out.status=AiPositionMoveStatus::InvalidGoal;return out;
    }
    rn::MovementFieldOptions free{false,true,(u.ai.policy_flags&0x8000u)!=0,(u.ai.policy_flags&0x10000u)!=0};
    const auto planning=detail::BuildOrdinaryAiField(r,slot,u.x,u.y,100,free);
    if(planning.get(gx,gy)<0) {out.status=AiPositionMoveStatus::GoalUnreachable;return out;}
    return detail::ExecuteOrdinaryMoveToResolved(r,slot,gx,gy);
}
AiPositionMoveResult detail::ExecuteOrdinaryMoveToResolved(rn::NativeRuntime& r,std::uint16_t slot,int gx,int gy,bool retain_no_progress_rng) {
    AiPositionMoveResult out{};out.unit_slot=slot;
    if(slot>=r.game.units.size())return out;
    const auto& u=r.game.units[slot];const auto* mp=r.definitions.terrain_map();const auto* jp=r.definitions.FindJob(u.job_id);
    out.start_x=out.end_x=u.x;out.start_y=out.end_y=u.y;out.goal_x=std::int16_t(gx);out.goal_y=std::int16_t(gy);
    if(!Live(u) || u.action_committed || !mp || !jp || gx<int(mp->min_x) || gy<int(mp->min_y) || gx>=int(mp->max_x) || gy>=int(mp->max_y))return out;
    const auto& map=*mp;const auto& job=*jp;
    rn::MovementFieldOptions free{false,true,(u.ai.policy_flags&0x8000u)!=0,(u.ai.policy_flags&0x10000u)!=0};
    const auto actual=detail::BuildOrdinaryAiField(r,slot,u.x,u.y,job.movement,{true,true,false,false});
    free.block_hostiles=(u.ai.policy_flags&0x400u)!=0;
    auto reverse=detail::BuildOrdinaryAiField(r,slot,gx,gy,100,free);
    if(reverse.get(u.x,u.y)<0 && free.block_hostiles) {
        free.block_hostiles=false;reverse=detail::BuildOrdinaryAiField(r,slot,gx,gy,100,free);out.reverse_retry_without_hostile_block=true;
    }
    int origin=reverse.get(u.x,u.y);
    if(origin<0) {
        origin=100;constexpr int dx[]{1,-1,0,0},dy[]{0,0,1,-1};
        for(int k=0;k<4;++k){const int c=reverse.get(u.x+dx[k],u.y+dy[k]);if(c>=0)origin=std::min(origin,c+1);}
    }
    std::uint32_t best=std::uint32_t(100-origin)<<19u;bool selected=false;int sx=u.x,sy=u.y;
    // Keep tie RNG and later obstacle rejection atomic with the movement.
    auto staged=std::make_unique<rn::NativeRuntime>(r);const auto ai_before=r.game.rng.ai;
    for(int y=int(map.min_y);y<int(map.max_y);++y)for(int x=int(map.min_x);x<int(map.max_x);++x) {
        const auto i=std::size_t(y*32+x);const int ac=actual.get(x,y),rc=reverse.get(x,y);
        if(ac<0 || rc<0 || !actual.stop[i] || (x==u.x && y==u.y))continue;
        const auto* t=r.definitions.TerrainAt(x,y);if(!t)continue;
        const auto score=OrdinaryMoveToScore(ac,rc,OrdinaryTerrainScore(t->defense_bonus,t->avoid_bonus,t->healing_bonus),
                                           detail::OrdinaryAdjacentAllyCount(r,slot,x,y),gx,gy,x,y);
        ++out.candidate_count;if(score<best)continue;
        if(score==best) {
            if(!staged->game.rng.ai_state.initialized) {out.status=AiPositionMoveStatus::MissingAiRandom;return out;}
            ++out.tie_draws;if(DrawAiLocalRandomBoundedExact(staged->game,2u)!=0u)continue;
        }
        selected=true;best=score;sx=x;sy=y;
    }
    if(!selected) {
        if(retain_no_progress_rng) {
            r.game.rng.ai_state=staged->game.rng.ai_state;r.game.rng.ai=staged->game.rng.ai;
        }
        out.status=AiPositionMoveStatus::NoProgress;return out;
    }
    // MoveTo may replace movement with ToDestroy/ToBreak. Preserve that
    // separate contract whenever a potentially qualifying obstacle exists.
    const int remaining=reverse.get(sx,sy);
    int max_range=0;
    const auto inventory=rn::ReadCalculationInventory(u);
    if(!inventory){out.status=AiPositionMoveStatus::UnsupportedProfile;return out;}
    for(auto instance:*inventory) {const auto item_id=instance.item_id;
        const auto* item=r.definitions.FindItem(item_id);if(item)max_range=std::max(max_range,int(item->max_range));
    }
    if(u.ai.policy_flags&0x8000u) for(int y=int(map.min_y);y<int(map.max_y);++y)for(int x=int(map.min_x);x<int(map.max_x);++x) {
        const int d=std::abs(x-sx)+std::abs(y-sy);const auto* t=r.definitions.TerrainAt(x,y);
        if(t && d>=1 && d<=max_range && reverse.get(x,y)<remaining && (t->flags_0x18&3u)) {
            out.status=AiPositionMoveStatus::UnsupportedObstacleAction;return out;
        }
    }
    if(u.ai.policy_flags&0x200u)for(const auto& other:r.game.units) {
        if(Live(other) && other.force_type!=u.force_type && std::abs(int(other.x)-sx)+std::abs(int(other.y)-sy)==1 &&
           reverse.get(other.x,other.y)<remaining) {out.status=AiPositionMoveStatus::UnsupportedObstacleAction;return out;}
    }
    if(rn::CommitUnitMove(*staged,slot,std::int16_t(sx),std::int16_t(sy))!=rn::PlayerActionStatus::Ok) {
        out.status=AiPositionMoveStatus::MovementRejected;return out;
    }
    staged->game.units[slot].action_committed=true;
    out.ai_rng_draws=staged->game.rng.ai-ai_before;r.game=std::move(staged->game);
    out.end_x=std::int16_t(sx);out.end_y=std::int16_t(sy);out.selected_score=best;out.moved=true;
    out.status=AiPositionMoveStatus::Ok;return out;
}

} // namespace fates::ai::native
