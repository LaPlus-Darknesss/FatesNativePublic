#include "fates/event/native_phase_event_queries.hpp"
#include "fates/runtime/native_runtime.hpp"
#include "fates/runtime/native_force_order.hpp"
#include "fates/runtime/native_game_user_settings.hpp"

namespace fates::event::native {
namespace rn=runtime::native;
namespace {
bool SameSituation(const rn::SituationPhaseState& a,const rn::SituationPhaseState& b) noexcept {
    return a.control==b.control && a.active_force==b.active_force && a.human_force==b.human_force &&
        a.turn==b.turn && a.turn_limit==b.turn_limit;
}
bool BoundPhase(const rn::NativeRuntime& r) noexcept {
    const auto& p=r.game.phase;
    return r.game.map_active && p.stage!=rn::PhaseAccessStage::Unbound &&
        p.stage<=rn::PhaseAccessStage::Terminal && p.revision!=0 &&
        p.chapter_index==r.game.campaign.current_chapter_index &&
        p.situation.active_force<3 && p.situation.human_force<3 && p.situation.turn>0 && p.situation.turn<=999;
}
}
PhaseQueryStatus NativePhaseEventQueries::Bind(std::shared_ptr<rn::NativeRuntime> runtime,
    std::shared_ptr<NativePhaseEventQueries>& out) {
    if(!runtime)return PhaseQueryStatus::NullRuntime;
    if(!BoundPhase(*runtime))return PhaseQueryStatus::UnboundPhase;
    if(!runtime->PlayerEvents() || !runtime->PlayerEvents()->active())return PhaseQueryStatus::StalePhase;
    auto next=std::shared_ptr<NativePhaseEventQueries>(new NativePhaseEventQueries);
    next->phase_=runtime->game.phase;next->player_=runtime->PlayerEvents();
    next->runtime_=std::move(runtime);out=std::move(next);
    return PhaseQueryStatus::Ok;
}
std::optional<PhaseIntegerQuerySpec> NativePhaseEventQueries::Find(std::string_view name) noexcept {
    if(name=="ev::DifficultyGet")return PhaseIntegerQuerySpec{PhaseIntegerQuery::Difficulty,0};
    if(name=="ev::TurnGet")return PhaseIntegerQuerySpec{PhaseIntegerQuery::Turn,0};
    if(name=="ev::ForceGetActive")return PhaseIntegerQuerySpec{PhaseIntegerQuery::ActiveForce,0};
    if(name=="ev::ForceUnitGetCount")return PhaseIntegerQuerySpec{PhaseIntegerQuery::ForceCount,1};
    return {};
}
PhaseQueryStatus NativePhaseEventQueries::Validate() const noexcept {
    if(retired_)return PhaseQueryStatus::Retired;
    const auto& r=*runtime_;const auto& p=r.game.phase;
    if(r.PlayerEvents()!=player_ || !player_ || !player_->active())return PhaseQueryStatus::StalePhase;
    if(!BoundPhase(r) || p.revision!=phase_.revision || p.chapter_index!=phase_.chapter_index ||
        p.stage!=phase_.stage || !SameSituation(p.situation,phase_.situation))return PhaseQueryStatus::StalePhase;
    return PhaseQueryStatus::Ok;
}
PhaseQueryStatus NativePhaseEventQueries::Query(PhaseIntegerQuery query,std::span<const std::int32_t> args,
    std::int32_t& out) const noexcept {
    const auto valid=Validate();if(valid!=PhaseQueryStatus::Ok)return valid;
    const auto& r=*runtime_;
    if(query!=PhaseIntegerQuery::Difficulty && query!=PhaseIntegerQuery::Turn &&
        query!=PhaseIntegerQuery::ActiveForce && query!=PhaseIntegerQuery::ForceCount)return PhaseQueryStatus::Unimplemented;
    if(args.size()!=(query==PhaseIntegerQuery::ForceCount?1u:0u))return PhaseQueryStatus::ArgumentCount;
    if(query==PhaseIntegerQuery::Difficulty) {
        const auto difficulty=rn::CurrentGameUserDifficulty(r);
        if(!difficulty)return PhaseQueryStatus::UnknownDifficulty;
        out=*difficulty;
    } else if(query==PhaseIntegerQuery::Turn) out=r.game.phase.situation.turn;
    else if(query==PhaseIntegerQuery::ActiveForce) out=r.game.phase.situation.active_force;
    else {
        // Original callback compares the full unsigned argument against9/10
        // BEFORE narrowing for Force::Get. Negative values also return zero.
        const auto force=static_cast<std::uint32_t>(args[0]);
        if(force>=9)out=0;
        else {
            const auto* order=rn::GetVerifiedForceOrder(r.game,static_cast<std::uint8_t>(force));
            if(!order)return PhaseQueryStatus::UnboundForceOrder;
            // Every linked member counts, regardless of HP, position or action.
            out=order->count;
        }
    }
    return PhaseQueryStatus::Ok;
}
}
