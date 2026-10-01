#include "fates/runtime/native_player_sortie.hpp"
#include "fates/runtime/native_force_order.hpp"
#include "fates/map/native_deployment_semantics.hpp"
#include <algorithm>
#include <array>
#include <set>
#include <utility>

namespace fates::runtime::native {
namespace {
bool PlayerIdentity(const PersonDefinition& p) {return (p.bitflags[0]&4u)!=0;}
}
PlayerSortieResult PlanExistingPlayerSortie(
    const NativeRuntime& r,const fates::headless::Fe14DisposGroupProjection& group,
    ExistingPlayerSortieContext context,std::span<const std::uint16_t> order) {
    PlayerSortieResult out;
    const auto difficulty=context.difficulty;
    auto fail=[&](PlayerSortieStatus status,std::size_t record=0xFFFFu) {
        out.status=status;out.failed_record=std::uint16_t(record);
        out.assignments.clear();out.unfilled_deployment_records.clear();out.unused_reserve_slots.clear();return out;
    };
    if(context.game_mode>=4 || context.calculate_flags!=0) return fail(PlayerSortieStatus::UnsupportedContext);
    if(!GameUserDifficultyMatches(r,difficulty)) return fail(PlayerSortieStatus::InvalidDifficulty);
    if(r.game.phase.stage!=PhaseAccessStage::Unbound) return fail(PlayerSortieStatus::PhaseAlreadyBound);
    if(group.spawns.empty()) return fail(PlayerSortieStatus::MissingGroup);
    if(group.spawns.size()>250 || order.size()>250) return fail(PlayerSortieStatus::UnsupportedRecord);
    std::array<bool,250> supplied{},used{};
    for(auto slot:order) {
        if(slot>=r.game.units.size() || supplied[slot]) return fail(PlayerSortieStatus::InvalidReserveOrder);
        supplied[slot]=true;const auto& u=r.game.units[slot];
        if(!u.occupied || u.force_type!=3 || u.has_position || u.defeated || u.pair.bound || u.pair.role!=PairRole::None)
            return fail(PlayerSortieStatus::InvalidReserveOrder);
        if(!r.definitions.FindPerson(u.person_id) || !r.definitions.FindJob(u.job_id)) return fail(PlayerSortieStatus::MissingDefinition);
        if(u.flags&(0x40000u|0x10000000u)) return fail(PlayerSortieStatus::UnsupportedIdentity);
    }
    const auto* terrain=r.definitions.terrain_map();
    if(!r.game.map_active || !terrain) return fail(PlayerSortieStatus::InvalidDestination);
    std::set<std::pair<int,int>> occupied;
    unsigned active_count=0;
    for(const auto& u:r.game.units) if(u.occupied) {
        if(u.force_type==0) ++active_count;
        if(u.force_type<3 && u.has_position && !u.defeated && u.pair.role!=PairRole::Partner) occupied.emplace(u.x,u.y);
    }
    // Validate all enabled records first. Only the fixed Player/Forced and
    // Deployment Slot variants are promoted. Other group/flag behavior stops.
    for(std::size_t i=0;i<group.spawns.size();++i) {
        const auto& s=group.spawns[i];
        if(!fates::map::native::DisposDifficultyAllows(difficulty,s.spawn_flags)) continue;
        const auto kind=s.spawn_flags&0x7Cu;
        if(s.runtime_state!=0 || s.team!=0 || (s.spawn_flags&~0x372Cu)!=0 ||
           (kind!=4 && kind!=8 && kind!=12 && kind!=32) ||
           (kind==32 && (s.spawn_flags&0x1000u)!=0) ||
           s.coord1_x!=s.coord2_x || s.coord1_y!=s.coord2_y)
            return fail(PlayerSortieStatus::UnsupportedRecord,i);
        if((kind&0x1Cu)!=0 && !r.definitions.FindPerson(s.pid)) return fail(PlayerSortieStatus::MissingDefinition,i);
    }
    for(unsigned pass=0;pass<2;++pass) for(std::size_t i=0;i<group.spawns.size();++i) {
        const auto& s=group.spawns[i];
        if(!fates::map::native::DisposDifficultyAllows(difficulty,s.spawn_flags)) continue;
        const bool fixed=(s.spawn_flags&0x1Cu)!=0;
        if(fixed!=(pass==0)) continue;
        const auto* authored=r.definitions.FindPerson(s.pid);
        // Replacement-person resolution is a separate runtime lookup contract.
        if(fixed && authored->replacing!=0) return fail(PlayerSortieStatus::UnsupportedIdentity,i);
        std::uint16_t selected=0xFFFFu;bool player_match=false;
        for(auto slot:order) {
            if(used[slot]) continue;
            const auto& u=r.game.units[slot];const auto* actual=r.definitions.FindPerson(u.person_id);
            if(fixed) {
                const bool is_player=PlayerIdentity(*authored);
                const auto* job=r.definitions.FindJob(u.job_id);
                const bool actual_player=((actual->bitflags[0]|job->bitflags[0]|u.private_skill_bits[0])&4u)!=0;
                if(is_player?!actual_player:u.person_id!=authored->id) continue;
                selected=slot;player_match=is_player;break;
            }
            if((u.flags&0x800u)==0) {selected=slot;break;}
        }
        if(selected==0xFFFFu) {
            if(fixed) return fail(PlayerSortieStatus::MissingFixedUnit,i);
            out.unfilled_deployment_records.push_back(std::uint16_t(i));continue;
        }
        if(active_count>49) return fail(PlayerSortieStatus::ForceCapacity,i);
        const int x=s.coord2_x,y=s.coord2_y;
        if(x<int(terrain->min_x) || y<int(terrain->min_y) || x>=int(terrain->max_x) || y>=int(terrain->max_y))
            return fail(PlayerSortieStatus::InvalidDestination,i);
        const auto* tile=r.definitions.TerrainAt(x,y);
        if(!tile || tile->movement_cost_index==0 || (tile->flags_0x18&8u)!=0)
            return fail(PlayerSortieStatus::InvalidDestination,i);
        if(!occupied.emplace(x,y).second) return fail(PlayerSortieStatus::OccupiedDestination,i);
        used[selected]=true;++active_count;
        out.assignments.push_back({std::uint16_t(i),selected,authored?authored->id:std::uint16_t(0),r.game.units[selected].person_id,
            std::int16_t(x),std::int16_t(y),fixed,player_match,
            fixed && (!player_match || PlayerIdentity(*r.definitions.FindPerson(r.game.units[selected].person_id)))});
    }
    for(auto slot:order) if(!used[slot]) out.unused_reserve_slots.push_back(slot);
    out.status=PlayerSortieStatus::Ok;return out;
}
PlayerSortieResult DeployExistingPlayerSortie(
    NativeRuntime& r,const fates::headless::Fe14DisposGroupProjection& group,
    ExistingPlayerSortieContext context,std::span<const std::uint16_t> order) {
    auto out=PlanExistingPlayerSortie(r,group,context,order);
    if(out.status!=PlayerSortieStatus::Ok) return out;
    // No operation below allocates or can fail. Preserve every persistent field
    // except the exact force/coordinate/deployment flag writes owned here.
    std::array<std::uint16_t,250> appended{};
    for(std::size_t i=0;i<out.assignments.size();++i)appended[i]=out.assignments[i].unit_slot;
    // CalculateImpl passes Transfer(...,1), which dispatches JoinLast.
    // Fixed-identity pass followed by ordinary slots is therefore insertion order.
    AppendPlayerSortieForceOrder(r.game,std::span(appended.data(),out.assignments.size()));
    for(const auto& a:out.assignments) {
        auto& u=r.game.units[a.unit_slot];const auto& s=group.spawns[a.source_record];
        u.force_type=0;u.has_position=true;u.x=a.x;u.y=a.y;
        u.flags=fates::map::native::ResolveCreatedUnitFlags(u.flags,s.spawn_flags,0,a.unique_person_match);
    }
    out.committed=true;return out;
}
} // namespace fates::runtime::native
