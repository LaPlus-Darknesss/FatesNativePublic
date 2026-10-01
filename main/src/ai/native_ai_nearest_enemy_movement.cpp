#include "fates/runtime/native_item_inventory.hpp"
#include "fates/ai/native_ai_attack_viability.hpp"
#include "fates/ai/native_ai_nearest_enemy_movement.hpp"
#include "fates/ai/native_ai_movement_fields.hpp"
#include "fates/ai/native_ai_candidate_scoring.hpp"
#include "fates/ai/native_ai_action.hpp"
#include "fates/runtime/native_current_item_eligibility.hpp"
#include "fates/runtime/native_force_order.hpp"
#include <algorithm>
#include <array>
#include <cstdlib>
#include <memory>
namespace fates::ai::native {
namespace rn=fates::runtime::native;
namespace {
template<class T> bool Zero(const T& a){return std::all_of(a.begin(),a.end(),[](auto v){return v==0;});}
bool OrdinaryPrivateBits(std::array<std::uint8_t,8> bits) {
    // Paragon bitflags_4 bit3 = Enemy Only; it does not alter the recovered
    // movement-power, terrain-score or ordinary planning paths.
    bits[3]&=std::uint8_t(~8u);return Zero(bits);
}
bool Empty(const std::array<std::int16_t,4>& a){return std::all_of(a.begin(),a.end(),[](auto v){return v==-1;});}
bool Live(const rn::UnitState& u){return u.occupied && u.has_position && !u.defeated && u.force_type<3 && u.pair.role!=rn::PairRole::Partner;}
bool OrdinaryPairMovement(const rn::NativeRuntime& r,const rn::UnitState& u) {
    if(!u.pair.bound)return u.pair.role==rn::PairRole::None;
    if(u.pair.role!=rn::PairRole::Lead || u.pair.partner_slot>=r.game.units.size())return false;
    const auto& v=r.game.units[u.pair.partner_slot];const auto* j=r.definitions.FindJob(v.job_id);
    if(!v.occupied || v.defeated || !v.has_position || !v.pair.bound || v.pair.role!=rn::PairRole::Partner ||
       v.pair.partner_slot>=r.game.units.size() || &r.game.units[v.pair.partner_slot]!=&u ||
       v.force_type!=u.force_type || v.x!=u.x || v.y!=u.y || !j)return false;
    // GetMovePower 0052D720..0052D728 reads partner Job+0x3C as a
    // signed movement contribution. The shared current movement budget is
    // correct only when that source byte is zero; nonzero bonuses stay closed.
    return j->pair_up_bonuses[0]==0;
}
bool OrdinaryProfile(const rn::NativeRuntime& r,const rn::UnitState& u) {
    const auto* p=r.definitions.FindPerson(u.person_id);const auto* j=r.definitions.FindJob(u.job_id);
    if(!p || !j || !r.definitions.terrain_map() || j->movement==0 || j->movement>100)return false;
    // Explicit fresh ordinary locomotion domain; class categories are not
    // private flags. Dark-Mage/Monster categories are equipment metadata, not flight.
    if(!u.create_from_dispos_combat_init_bound || !OrdinaryPrivateBits(p->bitflags) || !OrdinaryPrivateBits(j->bitflags) ||
       !Zero(u.private_skill_bits) || !Zero(p->personal_skills) || !OrdinaryPairMovement(r,u))return false;
    if((r.definitions.BaseJobCategoryMask(*j)&~0x0210u)!=0)return false;
    // These resolved skill IDs do not enter GetMovePower's movement+1 /
    // amphibious branches, GetTerrainScore's Locktouch branch, or Cheer.
    // Combat effects are NOT thereby approved: viability is separate below.
    constexpr std::array<std::uint16_t,14> neutral{{1,10,11,12,13,14,15,64,76,89,101,102,103,127}};
    for(auto id:u.equipped_skill_ids)if(id && std::find(neutral.begin(),neutral.end(),id)==neutral.end())return false;
    if((u.flags&~0x8003u)!=0 || (u.ai.policy_flags&~0x0041E6FFu)!=0 || u.ai.move_limit_mode!=0 ||
       u.ai.move_limit_x1!=255 || u.ai.move_limit_y1!=255 || u.ai.move_limit_x2!=255 || u.ai.move_limit_y2!=255)return false;
    if(u.force_type!=1)return false; // clever/human and allied-force paths not promoted
    for(const auto& v:r.game.units)if(Live(v) && v.force_type>1)return false;
    const auto& map=*r.definitions.terrain_map();
    for(const auto& t:map.terrain_types)if((t.flags_0x18&1u) && t.change_id_2 &&
       std::none_of(map.terrain_types.begin(),map.terrain_types.end(),[&](const auto& z){return z.id==t.change_id_2;}))return false;
    return true;
}
AiNearestEnemyMoveStatus ResolveRange(const rn::NativeRuntime& r,const rn::UnitState& u,AiNearestEnemyPlanning& out) {
    const auto inventory=rn::ReadCalculationInventory(u);if(!inventory)return AiNearestEnemyMoveStatus::MissingItem;
    if(u.equipped_item_id && std::none_of(inventory->begin(),inventory->end(),[&](auto item){return item.item_id==u.equipped_item_id;}))return AiNearestEnemyMoveStatus::MissingItem;
    for(auto instance:*inventory) {const auto id=instance.item_id;
        if(!id)continue;
        const auto* item=r.definitions.FindItem(id);
        if(!item)return AiNearestEnemyMoveStatus::MissingItem;
        const auto* sub=r.definitions.FindItemSubKind(item->weapon_category);
        if(!sub)return AiNearestEnemyMoveStatus::UnsupportedItemRange;
        const auto group=sub->weapon_exp_group;
        // Keys and other nonweapons can select a different free-move/door path.
        // Their presence is not silently ignored by this bounded mover.
        if(group>=8)return AiNearestEnemyMoveStatus::UnsupportedItemRange;
        const auto eligibility=rn::ProjectCurrentItemEligibility(r,u,*item,true,true);
        if(eligibility==rn::CurrentItemEligibility::No)continue;
        if(eligibility!=rn::CurrentItemEligibility::Yes)return AiNearestEnemyMoveStatus::UnsupportedItemRange;
        if(group==6)return AiNearestEnemyMoveStatus::UnsupportedStaffTargeting;
        // GetRangeI/O use unsigned source bytes, including two special sentinel
        // values. This ordinary subset refuses sentinel/stat-derived ranges.
        const auto lo=std::uint8_t(item->min_range),hi=std::uint8_t(item->max_range);
        if(lo<1 || hi<lo || hi>31 || item->movement)return AiNearestEnemyMoveStatus::UnsupportedItemRange;
        for(unsigned d=lo;d<=hi;++d)out.range_mask|=std::uint32_t(1)<<d;
        out.usable_inventory_ids.push_back(id);
    }
    return out.range_mask?AiNearestEnemyMoveStatus::Ok:AiNearestEnemyMoveStatus::MissingItem;
}
AiNearestEnemyPlanning Inspect(const rn::NativeRuntime& r,std::uint16_t slot,bool retry) {
    AiNearestEnemyPlanning out{};if(slot>=r.game.units.size())return out;
    const auto& u=r.game.units[slot];if(!Live(u) || u.action_committed)return out;
    if(!u.ai.configured || !u.ai.runtime_tuning_bound || u.ai.mission_id!=kMissionNull ||
       u.ai.movement_id!=kMovementNearestEnemy || !Empty(u.ai.mission_args) || !Empty(u.ai.movement_args) ||
       !Empty(u.ai.attack_args) || (u.ai.attack_id!=kAttackNull && u.ai.attack_id!=kAttackAttack) || !OrdinaryProfile(r,u)) {
        out.status=AiNearestEnemyMoveStatus::UnsupportedProfile;return out;
    }
    out.move_power=r.definitions.FindJob(u.job_id)->movement;
    out.requires_attack_viability=(u.ai.policy_flags&0x40u)==0;
    out.status=ResolveRange(r,u,out);if(out.status!=AiNearestEnemyMoveStatus::Ok)return out;
    std::vector<std::uint16_t> targets;
    for(std::uint16_t i=0;i<r.game.units.size();++i) {
        const auto& v=r.game.units[i];if(!Live(v) || v.force_type!=0)continue;
        if(v.flags&4u)continue; // IsAttackPermission excludes a paired child
        if(v.flags&0x400000u){out.status=AiNearestEnemyMoveStatus::UnsupportedTargetPermission;return out;}
        targets.push_back(i);
    }
    if(targets.empty()){out.status=AiNearestEnemyMoveStatus::NoHostileTarget;return out;}
    if(targets.size()>1) {
        if(!rn::PlayerForceOrderMatches(r.game)){out.status=AiNearestEnemyMoveStatus::AmbiguousRetailTargetOrder;return out;}
        const auto& order=r.game.player_force_order;
        std::sort(targets.begin(),targets.end(),[&](auto a,auto b){
            const auto end=order.slots.begin()+order.count;
            return std::find(order.slots.begin(),end,a)<std::find(order.slots.begin(),end,b);
        });
    }
    out.planning_blocks_hostiles=!retry && (u.ai.policy_flags&0x400u)!=0;
    const auto field=detail::BuildOrdinaryAiField(r,slot,u.x,u.y,100,
        {out.planning_blocks_hostiles,true,(u.ai.policy_flags&0x8000u)!=0,(u.ai.policy_flags&0x10000u)!=0});
    const auto& map=*r.definitions.terrain_map();
    for(auto target:targets) {
        const auto& v=r.game.units[target];AiNearestPlanningTarget t{};t.target_slot=target;t.history_count=r.game.ai_planner.target_move_counts[std::size_t(target)+1];
        for(int y=int(map.min_y);y<int(map.max_y);++y)for(int x=int(map.min_x);x<int(map.max_x);++x) {
            const int cost=field.get(x,y),dist=std::abs(x-int(v.x))+std::abs(y-int(v.y));
            if(cost<0 || cost>100 || dist<1 || dist>31 || !(out.range_mask&(std::uint32_t(1)<<unsigned(dist))))continue;
            t.cells.push_back({std::int16_t(x),std::int16_t(y),std::int16_t(cost)});
        }
        out.targets.push_back(std::move(t));
    }
    return out;
}
}
AiNearestEnemyPlanning InspectNearestEnemyPlanning(const rn::NativeRuntime& r,std::uint16_t slot){return Inspect(r,slot,false);}
std::uint32_t OrdinaryNearestTargetScore(std::uint8_t history,std::uint16_t cost,std::uint8_t move_power) noexcept {
    if(!move_power)return 0xFFFFFFFFu;
    return ((std::uint32_t(history)+cost)/move_power)*256u+history;
}
AiPlanningPositionSelection SelectNearestPlanningPosition(
    const std::span<const AiPlanningAttackCell> cells,rn::NativeGameState& state) {
    AiPlanningPositionSelection out{};int best=0;
    for(const auto& cell:cells) {
        if(cell.cost<0 || cell.cost>100)continue;
        const int score=100-cell.cost;if(score<best)continue;
        if(score==best) {
            if(!state.rng.ai_state.initialized){out.status=AiNearestEnemyMoveStatus::MissingAiRandom;return out;}
            ++out.ties;if(DrawAiLocalRandomBoundedExact(state,2u)!=0)continue;
        }
        best=score;out.x=cell.x;out.y=cell.y;out.cost=cell.cost;out.status=AiNearestEnemyMoveStatus::Ok;
    }
    return out;
}
AiNearestTargetSelection SelectNearestTargetGeometry(std::span<const AiNearestPlanningTarget> targets,std::uint8_t move,rn::NativeGameState& state) {
    AiNearestTargetSelection out{};if(!move)return out;
    for(const auto& t:targets) {
        const auto position=SelectNearestPlanningPosition(t.cells,state);
        out.position_ties=std::uint16_t(out.position_ties+position.ties);
        if(position.status==AiNearestEnemyMoveStatus::MissingAiRandom) {out.status=position.status;return out;}
        if(position.status!=AiNearestEnemyMoveStatus::Ok)continue;
        const auto* selected=&position;
        const auto score=OrdinaryNearestTargetScore(t.history_count,std::uint16_t(selected->cost),move);
        if(score>out.score)continue;
        if(score==out.score) {
            if(!state.rng.ai_state.initialized){out.status=AiNearestEnemyMoveStatus::MissingAiRandom;return out;}
            ++out.target_ties;if(DrawAiLocalRandomBoundedExact(state,2u)!=0)continue;
        }
        out.status=AiNearestEnemyMoveStatus::Ok;out.score=score;out.target_slot=t.target_slot;
        out.attack_x=selected->x;out.attack_y=selected->y;
    }
    return out;
}
AiNearestEnemyMoveResult ExecuteNearestEnemyMovementOnly(rn::NativeRuntime& r,std::uint16_t slot) {
    AiNearestEnemyMoveResult out{};out.unit_slot=slot;if(slot>=r.game.units.size())return out;
    const auto& u=r.game.units[slot];out.start_x=out.end_x=u.x;out.start_y=out.end_y=u.y;
    auto planning=Inspect(r,slot,false);out.status=planning.status;if(out.status!=AiNearestEnemyMoveStatus::Ok)return out;
    auto staged=std::make_unique<rn::NativeRuntime>(r);const auto before=r.game.rng.ai;
    auto select=[&]() {
        return planning.requires_attack_viability
            ? SelectNearestTargetWithAttackViability(r,slot,planning,staged->game,out.viability)
            : SelectNearestTargetGeometry(planning.targets,planning.move_power,staged->game);
    };
    auto selected=select();
    if(selected.status==AiNearestEnemyMoveStatus::NoPlanningAttackPosition && planning.planning_blocks_hostiles) {
        planning=Inspect(r,slot,true);out.planning_retry_without_hostile_block=true;
        if(planning.status!=AiNearestEnemyMoveStatus::Ok){out.status=planning.status;return out;}
        const auto earlier_position_ties=selected.position_ties,earlier_target_ties=selected.target_ties;
        selected=select();
        selected.position_ties=std::uint16_t(selected.position_ties+earlier_position_ties);
        selected.target_ties=std::uint16_t(selected.target_ties+earlier_target_ties);
    }
    out.status=selected.status;if(out.status!=AiNearestEnemyMoveStatus::Ok)return out;
    out.target_slot=selected.target_slot;out.target_score=std::int32_t(selected.score);
    out.planning_position_tie_draws=selected.position_ties;out.target_equal_score_encounters=selected.target_ties;
    const auto& target=r.game.units[selected.target_slot];
    auto moved=detail::ExecuteOrdinaryMoveToResolved(*staged,slot,target.x,target.y,true);
    if(moved.status==AiPositionMoveStatus::NoProgress) {
        // Retail retries the selected attack square when MoveTo(target) returns 0.
        // Preserve no-progress attempt draws inside this isolated transaction;
        // a later refusal still leaves every live gameplay/RNG field unchanged.
        out.fallback_to_attack_position=true;
        out.movement_tile_tie_draws=std::uint16_t(moved.tie_draws);
        moved=detail::ExecuteOrdinaryMoveToResolved(*staged,slot,selected.attack_x,selected.attack_y,true);
    }
    if(moved.status!=AiPositionMoveStatus::Ok) {
        out.status=moved.status==AiPositionMoveStatus::MissingAiRandom?AiNearestEnemyMoveStatus::MissingAiRandom:
            moved.status==AiPositionMoveStatus::UnsupportedObstacleAction?AiNearestEnemyMoveStatus::UnsupportedObstacleAction:
            moved.status==AiPositionMoveStatus::NoProgress?AiNearestEnemyMoveStatus::NoMovementDestination:AiNearestEnemyMoveStatus::MovementRejected;
        return out;
    }
    auto& history=staged->game.ai_planner.target_move_counts[std::size_t(selected.target_slot)+1];
    history=std::uint8_t(unsigned(history)+1u);
    out.movement_tile_tie_draws=std::uint16_t(out.movement_tile_tie_draws+moved.tie_draws);out.ai_rng_draws=staged->game.rng.ai-before;
    out.end_x=moved.end_x;out.end_y=moved.end_y;out.moved=moved.moved;out.status=AiNearestEnemyMoveStatus::Ok;
    r.game=std::move(staged->game);return out;
}
AiNearestEnemyMoveResult ExecuteB007NearestEnemyMovementOnly(rn::NativeRuntime& r,std::uint16_t slot,bool) {
    return ExecuteNearestEnemyMovementOnly(r,slot);
}
}
