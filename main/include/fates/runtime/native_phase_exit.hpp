#pragma once
#include "fates/runtime/native_force_turn.hpp"
#include "fates/map/native_danger_image.hpp"
#include <memory>

namespace fates::runtime::native {
enum class PhaseExitStatus : std::uint8_t {
    Ready, Complete, NullRuntime, InvalidPhase, StaleContext, StalePlayerState,
    RevisionExhausted, UpkeepBlocked, DangerBlocked, FlagsBlocked
};
enum class PhaseExitStage : std::uint8_t { Upkeep, Danger, Situation, Branch, Finished };
struct PhaseExitObservation {
    PhaseExitStatus status{PhaseExitStatus::Ready};
    PhaseExitStage stage{PhaseExitStage::Upkeep};
    ForceTurnResult upkeep{};
    fates::map::native::CurrentDangerResult danger{};
    EventFlagStatus flags{EventFlagStatus::Ok};
    bool advanced{};
    fates::chapter::native::MapSequenceEndRoute route{};
};
// Retained body of Sequence::TurnEnd, AFTER the scheduler's Reinforce child.
// Completed services are not replayed when a later dependency blocks. This
// owner never runs entry work or opens player commands. The scheduler consumer
// must perform the returned label jump at the original descriptor position.
class NativePhaseExit final {
public:
    static PhaseExitStatus Create(std::shared_ptr<NativeRuntime>,std::unique_ptr<NativePhaseExit>&);
    NativePhaseExit(const NativePhaseExit&)=delete;
    NativePhaseExit& operator=(const NativePhaseExit&)=delete;
    PhaseExitObservation Run();
    const PhaseExitObservation& Observe() const noexcept {return observation_;}
private:
    NativePhaseExit()=default;
    PhaseExitStatus Validate() const noexcept;
    EventFlagStatus Flag(EventFlagOperation,std::string_view,EventFlagResult&);
    std::shared_ptr<NativeRuntime> runtime_;
    std::shared_ptr<const NativePlayerEventState> player_;
    TacticalPhaseContext phase_;
    PhaseExitObservation observation_;
};
}
