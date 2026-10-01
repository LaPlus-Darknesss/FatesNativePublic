#include "fates/runtime/native_player_command.hpp"
#include "fates/runtime/native_phase.hpp"
#include <algorithm>
#include <memory>

namespace fates::runtime::native {
namespace {
namespace bn=fates::battle::native;
PlayerCommandStatus ActorStatus(const NativeRuntime& r,PlayerControlContext c,std::uint16_t slot) {
    if(!c.commands_enabled || c.active_force>=2 || !PhaseAllowsPlayerCommands(r,c.active_force)) return PlayerCommandStatus::ControlDisabled;
    if(slot>=r.game.units.size()) return PlayerCommandStatus::InvalidUnit;
    const auto& u=r.game.units[slot];
    if(!u.occupied || !u.has_position || u.defeated || u.force_type>=2)
        return PlayerCommandStatus::InvalidUnit;
    if(u.force_type!=c.active_force) return PlayerCommandStatus::WrongForce;
    if(u.action_committed) return PlayerCommandStatus::AlreadyActed;
    // The underlying death/rematerialization path is B007-bounded. Do not
    // promote arbitrary player pairing by calling a permissive primitive.
    if(u.pair.bound || u.pair.role!=PairRole::None) return PlayerCommandStatus::UnsupportedPair;
    return PlayerCommandStatus::Ok;
}
PlayerCommandStatus DestinationStatus(const NativeRuntime& r,const PlayerCommand& c) {
    const auto* map=r.definitions.terrain_map();
    if(!map || c.destination_x<int(map->min_x) || c.destination_y<int(map->min_y) ||
       c.destination_x>=int(map->max_x) || c.destination_y>=int(map->max_y))
        return PlayerCommandStatus::InvalidDestination;
    const auto& unit=r.game.units[c.unit_slot];
    const auto* job=r.definitions.FindJob(unit.job_id);
    if(!job) return PlayerCommandStatus::InvalidDestination;
    const auto cells=EnumerateUnitMovementField(r,c.unit_slot,unit.x,unit.y,job->movement,{true,true,false,false});
    const auto cell=std::find_if(cells.begin(),cells.end(),[&](const auto& v){
        return v.x==c.destination_x && v.y==c.destination_y;
    });
    if(cell==cells.end()) return PlayerCommandStatus::InvalidDestination;
    return cell->occupiable?PlayerCommandStatus::Ok:PlayerCommandStatus::OccupiedDestination;
}
PlayerCommandResult Validate(const NativeRuntime& r,PlayerControlContext control,const PlayerCommand& c) {
    PlayerCommandResult out{};out.command=c;out.status=ActorStatus(r,control,c.unit_slot);
    if(out.status!=PlayerCommandStatus::Ok) return out;
    const auto& actor=r.game.units[c.unit_slot];
    if(c.requested_item_id && c.requested_item_id!=actor.equipped_item_id) {
        out.status=PlayerCommandStatus::UnsupportedEquipmentChange;return out;
    }
    out.status=DestinationStatus(r,c);if(out.status!=PlayerCommandStatus::Ok) return out;
    out.moved=actor.x!=c.destination_x || actor.y!=c.destination_y;
    if(c.kind==PlayerCommandKind::Wait) return out;
    if(c.kind!=PlayerCommandKind::Attack || c.target_slot>=r.game.units.size() || c.unit_slot==c.target_slot) {
        out.status=PlayerCommandStatus::InvalidTarget;return out;
    }
    const auto& target=r.game.units[c.target_slot];
    if(!target.occupied || !target.has_position || target.defeated || target.force_type>=2 ||
       target.force_type==actor.force_type) {
        out.status=PlayerCommandStatus::InvalidTarget;return out;
    }
    if(target.pair.bound || target.pair.role!=PairRole::None) {
        out.status=PlayerCommandStatus::UnsupportedPair;return out;
    }
    out.forecast=bn::PreviewOrdinaryBattleAt(r,c.unit_slot,c.target_slot,c.destination_x,c.destination_y);
    if(out.forecast.status!=bn::BattleTransactionStatus::Ok) out.status=PlayerCommandStatus::BattleRejected;
    return out;
}
}
PlayerCommandResult ForecastPlayerCommand(const NativeRuntime& r,PlayerControlContext c,const PlayerCommand& command) {
    return Validate(r,c,command);
}
PlayerCommandResult ExecutePlayerCommand(NativeRuntime& r,PlayerControlContext c,const PlayerCommand& command) {
    auto out=Validate(r,c,command);if(out.status!=PlayerCommandStatus::Ok) return out;
    // Stage on an isolated runtime using the SAME primitive implementations.
    // Rejection (including late battle rejection) cannot leak movement, HP,
    // debuffs, pair mutations, counters or RNG. Definitions remain unchanged.
    auto candidate=std::make_unique<NativeRuntime>(r);
    const auto move=CommitPlayerMove(*candidate,command.unit_slot,command.destination_x,command.destination_y);
    if(move!=PlayerActionStatus::Ok) {out.status=PlayerCommandStatus::InvalidDestination;return out;}
    if(command.kind==PlayerCommandKind::Attack) {
        out.battle=bn::ExecuteOrdinaryBattle(*candidate,command.unit_slot,command.target_slot);
        if(out.battle.status!=bn::BattleTransactionStatus::Ok) {
            out.status=PlayerCommandStatus::BattleRejected;return out;
        }
    }
    candidate->game.units[command.unit_slot].action_committed=true;
    r.game=std::move(candidate->game);out.action_committed=true;return out;
}
PlayerCommandStatus PlayerCommandSession::Select(const NativeRuntime& r,PlayerControlContext c,std::uint16_t slot) {
    const auto status=ActorStatus(r,c,slot);if(status!=PlayerCommandStatus::Ok) return status;
    const auto& u=r.game.units[slot];selection_=Selection{slot,u.person_id,u.job_id,u.equipped_item_id,
        u.force_type,u.x,u.y,u.x,u.y,r.game.phase.revision,r.game.phase.stage!=PhaseAccessStage::Unbound,u.inventory,r.game.game_user_difficulty,u.map_end_revision,u.lineage.revision,u.capabilities.revision,u.transfer.revision,r.game.unit_slot_generations[slot],u.pair_revision};return status;
}
PlayerCommandStatus PlayerCommandSession::CheckSelection(const NativeRuntime& r,PlayerControlContext c) const {
    if(!selection_) return PlayerCommandStatus::NoSelection;
    const auto& s=*selection_;
    if(s.lineage_revision!=r.game.units[s.slot].lineage.revision||s.capability_revision!=r.game.units[s.slot].capabilities.revision)return PlayerCommandStatus::StaleSelection;
    if(s.transfer_revision!=r.game.units[s.slot].transfer.revision)return PlayerCommandStatus::StaleSelection;
    if(s.pair_revision!=r.game.units[s.slot].pair_revision)return PlayerCommandStatus::StaleSelection;
    if(s.slot_generation!=r.game.unit_slot_generations[s.slot])return PlayerCommandStatus::StaleSelection;
    if(s.map_end_revision!=r.game.units[s.slot].map_end_revision)return PlayerCommandStatus::StaleSelection;
    if(s.difficulty!=r.game.game_user_difficulty)return PlayerCommandStatus::StaleSelection;
    if(s.phase_revision!=r.game.phase.revision || s.phase_bound!=(r.game.phase.stage!=PhaseAccessStage::Unbound))
        return PlayerCommandStatus::StaleSelection;
    const auto status=ActorStatus(r,c,s.slot);
    if(status!=PlayerCommandStatus::Ok) return status;
    const auto& u=r.game.units[s.slot];
    if(u.person_id!=s.person || u.job_id!=s.job || u.equipped_item_id!=s.equipped ||
       u.inventory!=s.inventory || u.force_type!=s.force || u.x!=s.origin_x || u.y!=s.origin_y) return PlayerCommandStatus::StaleSelection;
    return PlayerCommandStatus::Ok;
}
PlayerCommand PlayerCommandSession::MakeCommand(PlayerCommandKind k,std::uint16_t target,std::uint16_t item) const {
    const auto& s=*selection_;return {k,s.slot,target,s.destination_x,s.destination_y,item};
}
PlayerCommandStatus PlayerCommandSession::StageMove(const NativeRuntime& r,PlayerControlContext c,std::int16_t x,std::int16_t y) {
    auto status=CheckSelection(r,c);if(status!=PlayerCommandStatus::Ok) return status;
    auto command=MakeCommand(PlayerCommandKind::Wait,0xFFFFu,0);command.destination_x=x;command.destination_y=y;
    status=DestinationStatus(r,command);
    if(status==PlayerCommandStatus::Ok) {selection_->destination_x=x;selection_->destination_y=y;}
    return status;
}
PlayerCommandResult PlayerCommandSession::Forecast(const NativeRuntime& r,PlayerControlContext c,
    PlayerCommandKind k,std::uint16_t target,std::uint16_t item) const {
    const auto status=CheckSelection(r,c);
    if(status!=PlayerCommandStatus::Ok) {PlayerCommandResult out{};out.status=status;return out;}
    return ForecastPlayerCommand(r,c,MakeCommand(k,target,item));
}
PlayerCommandResult PlayerCommandSession::Confirm(NativeRuntime& r,PlayerControlContext c,
    PlayerCommandKind k,std::uint16_t target,std::uint16_t item) {
    const auto status=CheckSelection(r,c);
    if(status!=PlayerCommandStatus::Ok) {PlayerCommandResult out{};out.status=status;return out;}
    auto out=ExecutePlayerCommand(r,c,MakeCommand(k,target,item));
    if(out.status==PlayerCommandStatus::Ok) Cancel();
    return out;
}
} // namespace fates::runtime::native
