#include "fates/runtime/native_force_order.hpp"
#include "fates/runtime/native_unit_pool.hpp"
#include "fates/runtime/native_unit_pair.hpp"
#include <memory>
#include "fates/ai/native_ai_state.hpp"
#include "fates/ai/native_ai_b007_original_deployment.hpp"
#include "fates/ai/native_dispos_combat_initialization.hpp"

namespace fates::ai::native {
namespace {
constexpr std::uint32_t kPairLeadFlag = 0x00010000u;
constexpr std::uint32_t kPairPartnerFlag = 0x00020000u;
constexpr std::size_t kB007EnemyCount = 19;

bool EnabledForDifficulty(const std::uint32_t flags,
                          const B007DeploymentDifficulty difficulty) noexcept {
    return (flags & B007DifficultySpawnMask(difficulty)) != 0;
}
} // namespace

std::uint32_t B007DifficultySpawnMask(const B007DeploymentDifficulty difficulty) noexcept {
    switch (difficulty) {
        case B007DeploymentDifficulty::Normal: return 0x100u;
        case B007DeploymentDifficulty::Hard: return 0x200u;
        case B007DeploymentDifficulty::Lunatic: return 0x400u;
    }
    return 0u;
}

B007OriginalEnemyDeploymentTopology BuildB007OriginalEnemyDeploymentTopology(
    const fates::headless::Fe14DisposFileProjection& dispos,
    const B007DeploymentDifficulty difficulty) {
    B007OriginalEnemyDeploymentTopology out{};
    out.difficulty = difficulty;
    const fates::headless::Fe14DisposGroupProjection* enemy = nullptr;
    for (const auto& group : dispos.groups) {
        if (group.name == "Enemy") { enemy = &group; break; }
    }
    if (!enemy) {
        out.status = B007DeploymentTopologyStatus::MissingEnemyGroup;
        return out;
    }
    out.source_enemy_records = static_cast<std::uint16_t>(enemy->spawns.size());
    if (enemy->spawns.size() != kB007EnemyCount) {
        out.status = B007DeploymentTopologyStatus::EnemyCountMismatch;
        return out;
    }

    out.records.resize(enemy->spawns.size());
    for (std::size_t i = 0; i < enemy->spawns.size(); ++i) {
        const auto& spawn = enemy->spawns[i];
        auto& rec = out.records[i];
        rec.source_record = static_cast<std::uint16_t>(i);
        rec.enabled = EnabledForDifficulty(spawn.spawn_flags, difficulty);
        if (!rec.enabled) {
            ++out.disabled_by_difficulty;
            continue;
        }
        ++out.enabled_records;
        if ((spawn.spawn_flags & kPairPartnerFlag) != 0) {
            rec.pair_role = B007PairRole::Partner;
            ++out.paired_partner_records;
            // Retail map::Dispos::Calculate never processes the 0x20000 record
            // standalone. The immediately preceding 0x10000 lead is calculated,
            // then this partner is CalculateImpl'd using the lead assignment and
            // Unit::DoubleOn copies the lead coordinates.
            if (i == 0 || !EnabledForDifficulty(enemy->spawns[i-1].spawn_flags, difficulty) ||
                (enemy->spawns[i-1].spawn_flags & kPairLeadFlag) == 0) {
                out.status = B007DeploymentTopologyStatus::MalformedPairTopology;
                return out;
            }
            rec.effective_x = enemy->spawns[i-1].coord2_x;
            rec.effective_y = enemy->spawns[i-1].coord2_y;
            continue;
        }

        rec.pair_role = (spawn.spawn_flags & kPairLeadFlag) != 0
            ? B007PairRole::Lead : B007PairRole::None;
        rec.effective_x = spawn.coord2_x;
        rec.effective_y = spawn.coord2_y;
        if (rec.effective_x < 0 || rec.effective_y < 0) {
            ++out.unpositioned_enabled_records;
            out.status = B007DeploymentTopologyStatus::InvalidStandalonePosition;
            return out;
        }
        out.phase_actor_source_records.push_back(static_cast<std::uint16_t>(i));

        if (rec.pair_role == B007PairRole::Lead) {
            if (i + 1 >= enemy->spawns.size()) {
                out.status = B007DeploymentTopologyStatus::MalformedPairTopology;
                return out;
            }
            const auto& partner = enemy->spawns[i+1];
            if (!EnabledForDifficulty(partner.spawn_flags, difficulty) ||
                (partner.spawn_flags & kPairPartnerFlag) == 0) {
                out.status = B007DeploymentTopologyStatus::MalformedPairTopology;
                return out;
            }
            out.pairs.push_back({static_cast<std::uint16_t>(i),
                                 static_cast<std::uint16_t>(i+1),
                                 spawn.coord2_x, spawn.coord2_y});
        }
    }
    out.phase_actor_records = static_cast<std::uint16_t>(out.phase_actor_source_records.size());
    out.status = B007DeploymentTopologyStatus::Ok;
    return out;
}

bool B007TopologyRequiresPairedBattleSupport(
    const B007OriginalEnemyDeploymentTopology& topology) noexcept {
    return topology.status == B007DeploymentTopologyStatus::Ok && !topology.pairs.empty();
}

B007PairBindStatus BindB007GuardStancePairs(
    fates::runtime::native::NativeGameState& state,
    const B007OriginalEnemyDeploymentTopology& topology,
    const std::array<std::uint16_t,19>& source_record_to_unit_slot) noexcept {
    using fates::runtime::native::PairRole;
    if (topology.status != B007DeploymentTopologyStatus::Ok)
        return B007PairBindStatus::InvalidTopology;
    auto staged=std::make_unique<fates::runtime::native::NativeGameState>(state);
    for (const auto& pair : topology.pairs) {
        if (pair.lead_record >= source_record_to_unit_slot.size() ||
            pair.partner_record >= source_record_to_unit_slot.size())
            return B007PairBindStatus::InvalidTopology;
        const auto lead_slot = source_record_to_unit_slot[pair.lead_record];
        const auto partner_slot = source_record_to_unit_slot[pair.partner_record];
        if (lead_slot >= state.units.size() || partner_slot >= state.units.size() ||
            lead_slot == partner_slot)
            return B007PairBindStatus::InvalidSlot;
        auto& lead = staged->units[lead_slot];
        auto& partner = staged->units[partner_slot];
        if (!lead.occupied || !partner.occupied) return B007PairBindStatus::MissingUnit;
        if (lead.force_type == 9 || partner.force_type == 9 || lead.force_type != partner.force_type)
            return B007PairBindStatus::InvalidPairForce;

        if(fates::runtime::native::LinkUnitPair(*staged,lead_slot,partner_slot)!=fates::runtime::native::UnitPairStatus::Ok)
            return B007PairBindStatus::InvalidTopology;
        // This existing original-data adapter supplies its proven zero bonuses.
        // Shared DoubleOn preserves gauge; fresh Unit creation already zeroed it.
        lead.pair.person_guard_bonus_bound=partner.pair.person_guard_bonus_bound=true;
        partner.has_position = true;
        partner.x = lead.x;
        partner.y = lead.y;
    }
    state=std::move(*staged);
    return B007PairBindStatus::Ok;
}

std::vector<std::uint16_t> BuildB007CurrentPhaseActorSlots(
    const fates::runtime::native::NativeGameState& state,
    const B007OriginalEnemyDeploymentTopology& topology,
    const std::array<std::uint16_t,19>& source_record_to_unit_slot) {
    using fates::runtime::native::PairRole;
    std::vector<std::uint16_t> out;
    if(topology.status!=B007DeploymentTopologyStatus::Ok || topology.records.size()!=source_record_to_unit_slot.size()) return out;
    for(const auto& rec:topology.records) {
        if(!rec.enabled || rec.source_record>=source_record_to_unit_slot.size()) continue;
        const auto slot=source_record_to_unit_slot[rec.source_record];
        if(slot>=state.units.size()) { out.clear(); return out; }
        const auto& unit=state.units[slot];
        if(!unit.occupied || !unit.has_position || unit.force_type==9 || unit.defeated ||
           unit.pair.role==PairRole::Partner) continue;
        out.push_back(slot);
    }
    return out;
}


B007OriginalPhaseExecutionResult ExecuteB007OriginalEnemyPhase(
    fates::runtime::native::NativeRuntime& runtime,
    const B007OriginalEnemyDeploymentTopology& topology,
    const std::array<std::uint16_t,19>& source_record_to_unit_slot,
    const std::uint16_t current_turn,
    const bool retail_enumeration_order_exact) {
    B007OriginalPhaseExecutionResult out{};
    if (topology.status != B007DeploymentTopologyStatus::Ok) {
        out.status = B007OriginalPhaseExecutionStatus::InvalidTopology;
        return out;
    }
    out.actor_slots = BuildB007CurrentPhaseActorSlots(runtime.game, topology, source_record_to_unit_slot);
    if (topology.enabled_records != 0 && out.actor_slots.empty()) {
        out.status = B007OriginalPhaseExecutionStatus::InvalidSourceMap;
        return out;
    }
    std::vector<AiPhaseRuntimeActorInput> seeds;
    seeds.reserve(out.actor_slots.size());
    for (const auto slot : out.actor_slots) {
        if (slot >= runtime.game.units.size()) {
            out.status = B007OriginalPhaseExecutionStatus::InvalidSourceMap;
            return out;
        }
        const auto& unit = runtime.game.units[slot];
        const auto* job = runtime.definitions.FindJob(unit.job_id);
        if (!job || job->movement == 0) {
            out.status = B007OriginalPhaseExecutionStatus::MissingJobDefinition;
            return out;
        }
        seeds.push_back({slot, static_cast<int>(job->movement)});
    }
    out.phase = ExecuteOrderedAttackPhaseFromRuntimeAiWithSharedOrdinaryPreview(
        runtime, seeds, current_turn, retail_enumeration_order_exact);
    if (out.phase.status != AiPhaseStatus::Ok ||
        out.phase.actor_binding_status != AiPhaseActorBindStatus::Ok) {
        out.status = B007OriginalPhaseExecutionStatus::PhaseRejected;
        return out;
    }
    out.status = B007OriginalPhaseExecutionStatus::Ok;
    return out;
}


namespace {
bool ParseSingleAiInt(const std::string& value, std::array<std::int16_t,4>& out) noexcept {
    out = {{-1,-1,-1,-1}};
    if (value.empty()) return true;
    int sign=1; std::size_t i=0; if(value[0]=='-'){sign=-1;i=1;} if(i>=value.size()) return false;
    int v=0; for(;i<value.size();++i){ if(value[i]<'0'||value[i]>'9') return false; v=v*10+(value[i]-'0'); if(v>32768) return false; }
    v*=sign; if(v<-32768||v>32767) return false; out[0]=static_cast<std::int16_t>(v); return true;
}
}

static B007OriginalDataImportResult ImportB007OriginalEnemyDisposStateImpl(
    fates::runtime::native::NativeGameState& state,
    const fates::runtime::native::DefinitionStore& definitions,
    const fates::headless::Fe14DisposFileProjection& dispos,
    const B007OriginalEnemyDeploymentTopology& topology) {
    B007OriginalDataImportResult out{};
    if(topology.status!=B007DeploymentTopologyStatus::Ok){out.status=B007OriginalDataImportStatus::InvalidTopology;return out;}
    const auto* enemy=dispos.FindGroup("Enemy");
    if(!enemy){out.status=B007OriginalDataImportStatus::MissingEnemyGroup;return out;}
    if(enemy->spawns.size()!=19 || topology.records.size()!=19){out.status=B007OriginalDataImportStatus::EnemyCountMismatch;return out;}
    for(std::size_t i=0;i<19;++i) out.source_record_to_unit_slot[i]=state.free_unit_pool.bound?0xffffu:static_cast<std::uint16_t>(i);
    for(std::size_t i=0;i<19;++i){
        const auto& rec=topology.records[i];
        const auto& src=enemy->spawns[i];
        if(!state.free_unit_pool.bound) {
            if(state.units[i].occupied){out.status=B007OriginalDataImportStatus::OccupiedDestination;return out;}
            state.units[i]={};
        }
        if(!rec.enabled) continue;
        const auto slot=state.free_unit_pool.bound?fates::runtime::native::FindFreeUnitPoolSlot(state).value_or(0xffffu):static_cast<std::uint16_t>(i);
        if(slot>=state.units.size()){out.status=B007OriginalDataImportStatus::ForceOrderRejected;return out;}
        auto& u=state.units[slot];u={};out.source_record_to_unit_slot[i]=slot;
        if(src.team==0||src.team>=3||src.runtime_state!=0||(src.spawn_flags&0x7cu)!=0){out.status=B007OriginalDataImportStatus::UnsupportedDeploymentContext;return out;}
        unsigned force_count=0;for(const auto& current:state.units)if(current.occupied&&current.force_type==src.team)++force_count;
        if(force_count>=50){out.status=B007OriginalDataImportStatus::ForceOrderRejected;return out;}
        const auto* person=definitions.FindPerson(src.pid);
        if(!person){out.status=B007OriginalDataImportStatus::MissingDefinition;return out;}
        const bool uses_default_job=src.job.empty();
        const auto* job=definitions.ResolveDisposJob(*person,src.job);
        if(!job){out.status=B007OriginalDataImportStatus::MissingDefinition;return out;}
        if(uses_default_job) ++out.default_job_fallback_units;
        InitializeFreshUnitAiState(u);BindDisposAiBand(u,src.ai_raw_76_78[0]);
        u.occupied=true; u.force_type=9; u.person_id=person->id; u.job_id=job->id;
        u.has_position=true; u.x=rec.effective_x; u.y=rec.effective_y;
        u.dispos_source_bound=true; u.dispos_source_record=static_cast<std::uint16_t>(i); u.dispos_authored_level=src.level;
        u.dispos_item_flags=src.item_flags;
        u.dispos_item_difficulty_adjustments=src.item_difficulty_adjustments;
        for(std::size_t k=0;k<5;++k){
            if(!src.items[k].empty()) { const auto* item=definitions.FindItem(src.items[k]); if(!item){out.status=B007OriginalDataImportStatus::MissingDefinition;return out;} u.dispos_item_ids[k]=item->id; }
            if(!src.skills[k].empty()) { const auto* skill=definitions.FindSkill(src.skills[k]); if(!skill){out.status=B007OriginalDataImportStatus::MissingDefinition;return out;} u.dispos_skill_ids[k]=skill->id; u.equipped_skill_ids[k]=skill->id; }
        }
        if(src.ai_action=="AI_AC_Everytime") u.ai.action_id=kActionEverytime;
        else if(src.ai_action=="AI_AC_TurnAttackRange") u.ai.action_id=kActionTurnAttackRange;
        else {out.status=B007OriginalDataImportStatus::UnsupportedAiDescriptor;return out;}
        if(src.ai_mission!="AI_MI_Null"||src.ai_attack!="AI_AT_Attack"||src.ai_movement!="AI_MV_NearestEnemy") {out.status=B007OriginalDataImportStatus::UnsupportedAiDescriptor;return out;}
        u.ai.mission_id=kMissionNull; u.ai.attack_id=kAttackAttack; u.ai.movement_id=kMovementNearestEnemy;
        if(!ParseSingleAiInt(src.ai_action_param,u.ai.action_args) || !src.ai_mission_param.empty() || !src.ai_attack_param.empty() || !src.ai_movement_param.empty()) {out.status=B007OriginalDataImportStatus::InvalidAiValue;return out;}
        u.ai.configured=true; u.ai.runtime_tuning_bound=true; u.ai.policy_flags=src.ai_policy; u.ai.priority=src.priority; u.ai.battle_rate=src.battle_rate;
        u.ai.move_limit_mode=src.move_limit[0];u.ai.move_limit_x1=src.move_limit[1];u.ai.move_limit_y1=src.move_limit[2];u.ai.move_limit_x2=src.move_limit[3];u.ai.move_limit_y2=src.move_limit[4];
        if(fates::runtime::native::JoinFreshUnitForceOrder(state,slot,src.team)!=fates::runtime::native::ForceOrderStatus::Ok){out.status=B007OriginalDataImportStatus::ForceOrderRejected;return out;}
        ++out.enabled_records; ++out.imported_units; if(u.combat_state_valid) ++out.combat_ready_units;
    }
    const auto pair_status=BindB007GuardStancePairs(state,topology,out.source_record_to_unit_slot);
    if(pair_status!=B007PairBindStatus::Ok){out.status=B007OriginalDataImportStatus::PairBindRejected;return out;}
    out.status=B007OriginalDataImportStatus::Ok; return out;
}

B007OriginalDataImportResult ImportB007OriginalEnemyDisposState(
    fates::runtime::native::NativeRuntime& runtime,
    const fates::headless::Fe14DisposFileProjection& dispos,
    const B007OriginalEnemyDeploymentTopology& topology) {
    auto staged=std::make_unique<fates::runtime::native::NativeGameState>(runtime.game);
    auto out=ImportB007OriginalEnemyDisposStateImpl(*staged,runtime.definitions,dispos,topology);
    if(out.status==B007OriginalDataImportStatus::Ok)runtime.game=std::move(*staged);
    else {out.imported_units=0;out.source_record_to_unit_slot.fill(0xffffu);}
    return out;
}


B007CombatInitResult InitializeB007OriginalEnemyCombatState(
    fates::runtime::native::NativeRuntime& runtime,
    const B007DeploymentDifficulty difficulty,
    const B007OriginalDataImportResult& imported) {
    const auto generic_difficulty = difficulty==B007DeploymentDifficulty::Normal
        ? NativeDisposDifficulty::Normal
        : (difficulty==B007DeploymentDifficulty::Hard ? NativeDisposDifficulty::Hard : NativeDisposDifficulty::Lunatic);
    const auto generic=InitializeDisposCombatState(runtime,generic_difficulty,imported.source_record_to_unit_slot);
    B007CombatInitResult out{};
    out.initialized_units=generic.initialized_units;out.equipped_units=generic.equipped_units;
    switch(generic.status){
        case NativeDisposCombatInitStatus::Ok: out.status=B007CombatInitStatus::Ok;break;
        case NativeDisposCombatInitStatus::MissingDefinition: out.status=B007CombatInitStatus::MissingDefinition;break;
        case NativeDisposCombatInitStatus::UnsupportedLevel: out.status=B007CombatInitStatus::UnsupportedLevel;break;
        case NativeDisposCombatInitStatus::InvalidItem: out.status=B007CombatInitStatus::InvalidItem;break;
        case NativeDisposCombatInitStatus::NoEquipableAuthoredWeapon: out.status=B007CombatInitStatus::NoEquipableAuthoredWeapon;break;
        case NativeDisposCombatInitStatus::InvalidDifficulty:
        case NativeDisposCombatInitStatus::UnsupportedItemVariant:
        case NativeDisposCombatInitStatus::UnsupportedEquipRestriction:
            out.status=B007CombatInitStatus::InvalidItem;break;
    }
    return out;
}

B007OriginalPhaseProbeResult ProbeB007OriginalEnemyPhase(
    fates::runtime::native::NativeRuntime& runtime,
    const fates::headless::Fe14DisposFileProjection& dispos,
    const B007DeploymentDifficulty difficulty) {
    B007OriginalPhaseProbeResult out{};
    const auto game_before=runtime.game.rng.game; const auto ai_before=runtime.game.rng.ai;
    const auto topology=BuildB007OriginalEnemyDeploymentTopology(dispos,difficulty);
    out.imported=ImportB007OriginalEnemyDisposState(runtime,dispos,topology);
    if(out.imported.status!=B007OriginalDataImportStatus::Ok){out.status=B007OriginalPhaseProbeStatus::ImportRejected;return out;}
    out.combat_init=InitializeB007OriginalEnemyCombatState(runtime,difficulty,out.imported);
    if(out.combat_init.status!=B007CombatInitStatus::Ok){out.status=B007OriginalPhaseProbeStatus::CombatInitializationRejected;return out;}
    out.actor_slots=BuildB007CurrentPhaseActorSlots(runtime.game,topology,out.imported.source_record_to_unit_slot);
    for(const auto slot:out.actor_slots) if(!runtime.game.units[slot].combat_state_valid) ++out.missing_combat_state;
    out.game_rng_draws=runtime.game.rng.game-game_before; out.ai_rng_draws=runtime.game.rng.ai-ai_before;
    out.status=out.missing_combat_state?B007OriginalPhaseProbeStatus::MissingCombatInitialization:B007OriginalPhaseProbeStatus::ReadyToExecute;
    return out;
}

} // namespace fates::ai::native
