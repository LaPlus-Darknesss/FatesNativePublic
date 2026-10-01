#include "fates/runtime/native_force_order.hpp"
#include "fates/runtime/native_unit_pool.hpp"
#include "fates/runtime/native_unit_pair.hpp"
#include <memory>
#include "fates/ai/native_ai_state.hpp"
#include "fates/ai/native_ai_original_enemy_dispos.hpp"
#include "fates/ai/native_ai_activation.hpp"
#include "fates/ai/native_ai_action.hpp"
#include <array>
#include <cctype>
#include <limits>
#include <string_view>

namespace fates::ai::native {
namespace {
constexpr std::uint32_t kPairMask=0x00030000u;
constexpr std::uint16_t kInvalidSlot=0xFFFFu;

bool ParseInt(std::string_view s,std::int16_t& out) noexcept {
    if(s.empty()) return false;
    int sign=1;std::size_t i=0;if(s[0]=='-'){sign=-1;++i;}if(i>=s.size())return false;
    int v=0;for(;i<s.size();++i){if(s[i]<'0'||s[i]>'9')return false;v=v*10+(s[i]-'0');if(v>32768)return false;}
    v*=sign;if(v<-32768||v>32767)return false;out=static_cast<std::int16_t>(v);return true;
}
bool ParseSingleOrEmpty(const std::string& s,std::array<std::int16_t,4>& out) noexcept {
    out={{-1,-1,-1,-1}};if(s.empty())return true;return ParseInt(s,out[0]);
}
bool ParsePositionOrEmpty(const std::string& s,std::array<std::int16_t,4>& out) noexcept {
    out={{-1,-1,-1,-1}};if(s.empty())return true;
    if(s.size()<7||s.rfind("pos(",0)!=0||s.back()!=')')return false;
    const auto comma=s.find(',',4);if(comma==std::string::npos)return false;
    std::int16_t x{},y{};if(!ParseInt(std::string_view(s).substr(4,comma-4),x))return false;
    if(!ParseInt(std::string_view(s).substr(comma+1,s.size()-comma-2),y))return false;
    out[0]=x;out[1]=y;return true;
}
std::uint16_t FindFreeSlot(const fates::runtime::native::NativeGameState& state) noexcept {
    return fates::runtime::native::FindFreeUnitPoolSlot(state).value_or(kInvalidSlot);
}
bool BindAi(const fates::headless::Fe14DisposSpawnProjection& src,
            fates::runtime::native::UnitState& u) noexcept {
    if(src.ai_action=="AI_AC_Everytime") u.ai.action_id=kActionEverytime;
    else if(src.ai_action=="AI_AC_Turn") u.ai.action_id=kActionTurn;
    else if(src.ai_action=="AI_AC_TurnAttackRange") u.ai.action_id=kActionTurnAttackRange;
    else return false;
    if(src.ai_mission=="AI_MI_Null"||src.ai_mission.empty()) u.ai.mission_id=kMissionNull;
    else return false;
    if(src.ai_attack=="AI_AT_Null"||src.ai_attack.empty()) u.ai.attack_id=kAttackNull;
    else if(src.ai_attack=="AI_AT_Attack") u.ai.attack_id=kAttackAttack;
    else return false;
    if(src.ai_movement=="AI_MV_Null"||src.ai_movement.empty()) u.ai.movement_id=kMovementNull;
    else if(src.ai_movement=="AI_MV_NearestEnemy") u.ai.movement_id=kMovementNearestEnemy;
    else if(src.ai_movement=="AI_MV_Position") u.ai.movement_id=kMovementPosition;
    else return false;
    if(!ParseSingleOrEmpty(src.ai_action_param,u.ai.action_args)) return false;
    if(!ParseSingleOrEmpty(src.ai_mission_param,u.ai.mission_args)) return false;
    if(!ParseSingleOrEmpty(src.ai_attack_param,u.ai.attack_args)) return false;
    if(u.ai.movement_id==kMovementPosition) {
        if(!ParsePositionOrEmpty(src.ai_movement_param,u.ai.movement_args)) return false;
    } else if(!ParseSingleOrEmpty(src.ai_movement_param,u.ai.movement_args)) return false;
    u.ai.configured=true;u.ai.runtime_tuning_bound=true;u.ai.policy_flags=src.ai_policy;
    u.ai.priority=src.priority;u.ai.battle_rate=src.battle_rate;
    u.ai.move_limit_mode=src.move_limit[0];u.ai.move_limit_x1=src.move_limit[1];u.ai.move_limit_y1=src.move_limit[2];
    u.ai.move_limit_x2=src.move_limit[3];u.ai.move_limit_y2=src.move_limit[4];
    return true;
}
}

std::uint32_t OriginalEnemyDifficultySpawnMask(const NativeDisposDifficulty difficulty) noexcept {
    switch(difficulty){case NativeDisposDifficulty::Normal:return 0x100u;case NativeDisposDifficulty::Hard:return 0x200u;case NativeDisposDifficulty::Lunatic:return 0x400u;}return 0u;
}

static OriginalEnemyImportResult ImportOriginalEnemyDisposStateImpl(
    fates::runtime::native::NativeGameState& state,
    const fates::runtime::native::DefinitionStore& definitions,
    const fates::headless::Fe14DisposFileProjection& dispos,
    const NativeDisposDifficulty difficulty) {
    OriginalEnemyImportResult out{};out.difficulty=difficulty;
    const auto* enemy=dispos.FindGroup("Enemy");if(!enemy){out.status=OriginalEnemyImportStatus::MissingEnemyGroup;return out;}
    out.source_enemy_records=static_cast<std::uint16_t>(enemy->spawns.size());
    out.source_record_to_unit_slot.assign(enemy->spawns.size(),kInvalidSlot);
    for(std::size_t i=0;i<enemy->spawns.size();++i){
        const auto& src=enemy->spawns[i];if((src.spawn_flags&OriginalEnemyDifficultySpawnMask(difficulty))==0)continue;
        ++out.enabled_records;
        if(src.team==0||src.team>=3||src.runtime_state!=0||(src.spawn_flags&0x7cu)!=0){out.status=OriginalEnemyImportStatus::UnsupportedDeploymentContext;return out;}
        unsigned force_count=0;for(const auto& current:state.units)if(current.occupied&&current.force_type==src.team)++force_count;
        if(force_count>=50){out.status=OriginalEnemyImportStatus::ForceOrderRejected;return out;}
        const auto pair_role=src.spawn_flags&kPairMask;
        const bool partner=pair_role==0x20000u,lead=pair_role==0x10000u;
        auto enabled=[&](std::size_t n){return n<enemy->spawns.size()&&(enemy->spawns[n].spawn_flags&OriginalEnemyDifficultySpawnMask(difficulty));};
        if(pair_role==kPairMask || (lead&&(!enabled(i+1)||(enemy->spawns[i+1].spawn_flags&kPairMask)!=0x20000u)) ||
           (partner&&(i==0||!enabled(i-1)||(enemy->spawns[i-1].spawn_flags&kPairMask)!=0x10000u||enemy->spawns[i-1].team!=src.team))) {
            out.status=OriginalEnemyImportStatus::UnsupportedPairTopology;return out;
        }
        const auto& position=partner?enemy->spawns[i-1]:src;
        if(position.coord2_x<0||position.coord2_y<0){out.status=OriginalEnemyImportStatus::InvalidPosition;return out;}
        const auto slot=FindFreeSlot(state);if(slot==kInvalidSlot){out.status=OriginalEnemyImportStatus::UnitPoolExhausted;return out;}
        const auto* person=definitions.FindPerson(src.pid);if(!person){out.status=OriginalEnemyImportStatus::MissingDefinition;return out;}
        const bool default_job=src.job.empty();const auto* job=definitions.ResolveDisposJob(*person,src.job);
        if(!job){out.status=OriginalEnemyImportStatus::MissingDefinition;return out;}if(default_job)++out.default_job_fallback_units;
        auto& u=state.units[slot];u={};InitializeFreshUnitAiState(u);BindDisposAiBand(u,src.ai_raw_76_78[0]);u.occupied=true;u.force_type=9;u.person_id=person->id;u.job_id=job->id;
        if(fates::runtime::native::BindUnitPersonRecord(definitions,u,definitions.RetainPerson(*person))!=fates::runtime::native::UnitPersonLookupStatus::Ok){out.status=OriginalEnemyImportStatus::MissingDefinition;return out;}
        u.has_position=true;u.x=position.coord2_x;u.y=position.coord2_y;u.sortie_order_key=static_cast<std::uint16_t>(i);
        u.dispos_source_bound=true;u.dispos_source_record=static_cast<std::uint16_t>(i);u.dispos_authored_level=src.level;
        u.dispos_item_flags=src.item_flags;
        u.dispos_item_difficulty_adjustments=src.item_difficulty_adjustments;
        for(std::size_t k=0;k<5;++k){
            if(!src.items[k].empty()){const auto* item=definitions.FindItem(src.items[k]);if(!item){out.status=OriginalEnemyImportStatus::MissingDefinition;return out;}u.dispos_item_ids[k]=item->id;}
            if(!src.skills[k].empty()){const auto* skill=definitions.FindSkill(src.skills[k]);if(!skill){out.status=OriginalEnemyImportStatus::MissingDefinition;return out;}u.dispos_skill_ids[k]=skill->id;u.equipped_skill_ids[k]=skill->id;}
        }
        if(!BindAi(src,u)){out.status=OriginalEnemyImportStatus::UnsupportedAiDescriptor;return out;}
        if(fates::runtime::native::JoinFreshUnitForceOrder(state,slot,src.team)!=fates::runtime::native::ForceOrderStatus::Ok){out.status=OriginalEnemyImportStatus::ForceOrderRejected;return out;}
        out.source_record_to_unit_slot[i]=slot;
        if(partner) {
            if(fates::runtime::native::LinkUnitPair(state,out.source_record_to_unit_slot[i-1],slot)!=fates::runtime::native::UnitPairStatus::Ok) {
                out.status=OriginalEnemyImportStatus::UnsupportedPairTopology;return out;
            }
            // Relationship bonuses require their own support projection. DoubleOn
            // establishes topology; it cannot invent zero bonuses for arbitrary data.
        } else out.phase_actor_slots.push_back(slot);
        ++out.imported_units;
    }
    return out;
}

OriginalEnemyImportResult ImportOriginalEnemyDisposState(
    fates::runtime::native::NativeRuntime& runtime,
    const fates::headless::Fe14DisposFileProjection& dispos,
    const NativeDisposDifficulty difficulty) {
    if(!fates::runtime::native::GameUserDifficultyMatches(runtime,static_cast<std::uint8_t>(difficulty))) {
        OriginalEnemyImportResult out{};out.difficulty=difficulty;out.status=OriginalEnemyImportStatus::UnsupportedDeploymentContext;return out;
    }
    auto staged=std::make_unique<fates::runtime::native::NativeGameState>(runtime.game);
    auto out=ImportOriginalEnemyDisposStateImpl(*staged,runtime.definitions,dispos,difficulty);
    if(out.status==OriginalEnemyImportStatus::Ok)runtime.game=std::move(*staged);
    else {out.imported_units=0;out.source_record_to_unit_slot.assign(out.source_record_to_unit_slot.size(),0xffffu);out.phase_actor_slots.clear();}
    return out;
}

OriginalEnemyCombatProbeResult ProbeOriginalEnemyCombatState(
    fates::runtime::native::NativeRuntime& runtime,
    const fates::headless::Fe14DisposFileProjection& dispos,
    const NativeDisposDifficulty difficulty){
    OriginalEnemyCombatProbeResult out{};out.imported=ImportOriginalEnemyDisposState(runtime,dispos,difficulty);
    if(out.imported.status!=OriginalEnemyImportStatus::Ok)return out;
    std::vector<std::uint16_t> imported;
    for(auto slot:out.imported.source_record_to_unit_slot)if(slot!=kInvalidSlot)imported.push_back(slot);
    out.combat_init=InitializeDisposCombatState(runtime,difficulty,imported);return out;
}

OriginalEnemyPhaseExecutionResult ExecuteOriginalEnemyPhase(
    fates::runtime::native::NativeRuntime& runtime,
    const fates::headless::Fe14DisposFileProjection& dispos,
    const NativeDisposDifficulty difficulty,
    const std::uint16_t current_turn,
    const bool retail_enumeration_order_exact){
    OriginalEnemyPhaseExecutionResult out{};
    auto probe=ProbeOriginalEnemyCombatState(runtime,dispos,difficulty);out.import_status=probe.imported.status;out.combat_status=probe.combat_init.status;
    out.actor_slots=probe.imported.phase_actor_slots;
    if(out.import_status!=OriginalEnemyImportStatus::Ok||out.combat_status!=NativeDisposCombatInitStatus::Ok){out.phase.status=AiPhaseStatus::ActionRejected;return out;}
    std::vector<AiPhaseRuntimeActorInput> seeds;seeds.reserve(out.actor_slots.size());
    for(const auto slot:out.actor_slots){const auto* job=runtime.definitions.FindJob(runtime.game.units[slot].job_id);if(!job||job->movement==0){out.phase.status=AiPhaseStatus::ActionRejected;return out;}seeds.push_back({slot,static_cast<int>(job->movement)});}
    out.phase=ExecuteOrderedAttackPhaseFromRuntimeAiWithSharedOrdinaryPreview(runtime,seeds,current_turn,retail_enumeration_order_exact);return out;
}

} // namespace fates::ai::native
