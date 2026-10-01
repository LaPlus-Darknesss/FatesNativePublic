#include "fates/runtime/native_player_action.hpp"
#include "fates/runtime/native_runtime.hpp"
#include "fates/map/native_movement_field.hpp"
#include <algorithm>
#include <array>

namespace fates::runtime::native {
namespace {
bool SameSide(std::uint8_t a, std::uint8_t b) noexcept { return a == b; }
const UnitState* UnitAt(const NativeGameState& s, int x, int y, std::uint16_t ignore) {
    for (std::uint16_t i=0;i<s.units.size();++i) {
        const auto& u=s.units[i];
        if (i==ignore || !u.occupied || !u.has_position || !fates::runtime::native::IsTacticalForce(u.force_type) || u.defeated || u.pair.role==PairRole::Partner) continue;
        if (u.x==x && u.y==y) return &u;
    }
    return nullptr;
}
}

std::vector<ReachableCell> EnumerateUnitMovementField(
    const NativeRuntime& runtime, const std::uint16_t unit_slot,
    const std::int16_t source_x, const std::int16_t source_y,
    const int movement_budget, const MovementFieldOptions options) {
    std::vector<ReachableCell> out;
    if (unit_slot>=runtime.game.units.size() || movement_budget<0) return out;
    const auto& u=runtime.game.units[unit_slot];
    if (!u.occupied || !fates::runtime::native::IsTacticalForce(u.force_type) || u.pair.role==PairRole::Partner) return out;
    const auto* job=runtime.definitions.FindJob(u.job_id);
    const auto* terrain=runtime.definitions.terrain_map();
    if (!job || !terrain) return out;
    const auto& costs=runtime.definitions.movement_costs();
    if (job->movement_cost_index>=costs.size()) return out;
    const auto& row=costs[job->movement_cost_index];
    const int w=static_cast<int>(terrain->width), h=static_cast<int>(terrain->height);
    if(w<1 || h<1 || w>32 || h>32) return out;
    const int min_x=options.active_rectangle_only?int(terrain->min_x):0;
    const int min_y=options.active_rectangle_only?int(terrain->min_y):0;
    const int max_x=options.active_rectangle_only?int(terrain->max_x):w;
    const int max_y=options.active_rectangle_only?int(terrain->max_y):h;
    if(min_x<0 || min_y<0 || max_x>w || max_y>h || min_x>=max_x || min_y>=max_y) return out;
    if(source_x<min_x || source_y<min_y || source_x>=max_x || source_y>=max_y) return out;
    const auto cost=[&](int nx,int ny)->std::optional<int> {
        auto* td=runtime.definitions.TerrainAt(nx,ny);if(!td)return -1;
        if(options.destroyed_terrain_projection&&(td->flags_0x18&1u)&&td->change_id_2) {
            const auto found=std::find_if(terrain->terrain_types.begin(),terrain->terrain_types.end(),
                [&](const auto& v){return v.id==td->change_id_2;});
            if(found==terrain->terrain_types.end())return std::nullopt;
            td=&*found;
        }
        if(td->movement_cost_index>=row.size())return -1;
        int ec=row[td->movement_cost_index];
        if(ec<0&&options.base_cost_fallback&&!costs.empty()&&td->movement_cost_index<costs[0].size())
            ec=costs[0][td->movement_cost_index];
        if(ec<0)return -1;
        const UnitState* occ=UnitAt(runtime.game,nx,ny,unit_slot);
        if(options.block_hostiles&&occ&&!SameSide(u.force_type,occ->force_type))return -1;
        return ec;
    };
    const auto field=fates::map::native::ComputeMovementCostField(
        {min_x,min_y,max_x,max_y},source_x,source_y,movement_budget,cost);
    if(!field)return out;
    for(int y=0;y<h;++y)for(int x=0;x<w;++x){int c=(*field)[y*32+x];if(c<0)continue;
        const UnitState* occ=UnitAt(runtime.game,x,y,unit_slot);
        out.push_back({static_cast<std::int16_t>(x),static_cast<std::int16_t>(y),
                       static_cast<std::int16_t>(c),!occ});
    }
    std::sort(out.begin(),out.end(),[](const auto&a,const auto&b){if(a.cost!=b.cost)return a.cost<b.cost;if(a.y!=b.y)return a.y<b.y;return a.x<b.x;});
    return out;
}

std::vector<ReachableCell> EnumerateUnitMovementFromWithBudget(
    const NativeRuntime& r,std::uint16_t slot,std::int16_t x,std::int16_t y,int budget) {
    return EnumerateUnitMovementField(r,slot,x,y,budget,{});
}

std::vector<ReachableCell> EnumerateUnitMovement(const NativeRuntime& runtime,
                                                  const std::uint16_t unit_slot) {
    if(unit_slot>=runtime.game.units.size()) return {};
    const auto& u=runtime.game.units[unit_slot];
    const auto* job=runtime.definitions.FindJob(u.job_id);
    if(!job || !u.has_position) return {};
    return EnumerateUnitMovementFromWithBudget(runtime,unit_slot,u.x,u.y,job->movement);
}

PlayerActionStatus CommitUnitMove(NativeRuntime& runtime,std::uint16_t slot,std::int16_t x,std::int16_t y){
    if(slot>=runtime.game.units.size())return PlayerActionStatus::InvalidUnit;
    auto& u=runtime.game.units[slot]; if(!u.occupied||!u.has_position||!fates::runtime::native::IsTacticalForce(u.force_type)||u.pair.role==PairRole::Partner)return PlayerActionStatus::InvalidUnit;
    auto cells=EnumerateUnitMovement(runtime,slot); auto it=std::find_if(cells.begin(),cells.end(),[&](const auto&c){return c.x==x&&c.y==y;});
    if(it==cells.end())return PlayerActionStatus::InvalidDestination;
    if(!it->occupiable)return PlayerActionStatus::OccupiedDestination;
    u.x=x;u.y=y;
    if(u.pair.bound && u.pair.role==PairRole::Lead && u.pair.partner_slot<runtime.game.units.size()) {
        auto& partner=runtime.game.units[u.pair.partner_slot];
        if(partner.pair.bound && partner.pair.role==PairRole::Partner && partner.pair.partner_slot==slot) {
            partner.has_position=true; partner.x=x; partner.y=y;
        }
    }
    return PlayerActionStatus::Ok;
}

std::vector<ReachableCell> EnumeratePlayerMovement(const NativeRuntime& runtime, std::uint16_t slot) {
    return EnumerateUnitMovement(runtime,slot);
}
PlayerActionStatus CommitPlayerMove(NativeRuntime& runtime,std::uint16_t slot,std::int16_t x,std::int16_t y) {
    return CommitUnitMove(runtime,slot,x,y);
}

std::vector<AttackTarget> EnumerateAttackTargets(const NativeRuntime& runtime,std::uint16_t slot,std::uint16_t item_id){
    std::vector<AttackTarget> out; if(slot>=runtime.game.units.size())return out;
    const auto& u=runtime.game.units[slot]; if(!u.occupied||!u.has_position||!fates::runtime::native::IsTacticalForce(u.force_type)||u.pair.role==PairRole::Partner)return out;
    const auto* item=runtime.definitions.FindItem(item_id); if(!item)return out;
    int lo=item->min_range, hi=item->max_range; if(lo<0||hi<lo)return out;
    for(std::uint16_t i=0;i<runtime.game.units.size();++i){if(i==slot)continue;const auto&t=runtime.game.units[i];
        if(!t.occupied||!t.has_position||!fates::runtime::native::IsTacticalForce(t.force_type)||t.pair.role==PairRole::Partner||SameSide(u.force_type,t.force_type))continue;
        int d=std::abs(int(u.x)-int(t.x))+std::abs(int(u.y)-int(t.y));
        if(d>=lo&&d<=hi)out.push_back({i,t.x,t.y,static_cast<std::uint8_t>(d)});
    }
    return out;
}

PlayerActionResult ExecuteMoveThenEnumerateAttack(NativeRuntime& runtime,std::uint16_t slot,std::int16_t x,std::int16_t y,std::uint16_t item){
    PlayerActionResult r{};r.unit_slot=slot;
    if(slot>=runtime.game.units.size()){r.status=PlayerActionStatus::InvalidUnit;return r;}
    auto& u=runtime.game.units[slot];r.start_x=u.x;r.start_y=u.y;r.reachable=EnumeratePlayerMovement(runtime,slot);
    r.status=CommitPlayerMove(runtime,slot,x,y);if(r.status!=PlayerActionStatus::Ok)return r;
    r.end_x=u.x;r.end_y=u.y;r.attack_targets=EnumerateAttackTargets(runtime,slot,item);u.action_committed=true;return r;
}
} // namespace fates::runtime::native
