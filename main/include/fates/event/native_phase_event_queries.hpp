#pragma once
#include "fates/runtime/native_phase_state.hpp"
#include <memory>
#include <optional>
#include <span>
#include <string_view>

namespace fates::runtime::native {struct NativeRuntime;class NativePlayerEventState;}
namespace fates::event::native {
enum class PhaseQueryStatus : std::uint8_t {
    Ok, NullRuntime, UnboundPhase, StalePhase, Retired, Unimplemented,
    ArgumentCount, UnknownDifficulty, UnboundForceOrder
};
enum class PhaseIntegerQuery : std::uint8_t { Difficulty, Turn, ActiveForce, ForceCount };
struct PhaseIntegerQuerySpec { PhaseIntegerQuery query; std::uint8_t arguments; };

// Concrete read-only commands over the existing native ownership root. This
// retains that root, not a snapshot of the query values. A binding belongs to
// one live phase context; retirement is permanent. It is not full script/session
// attachment, a SaveData provider, or an acknowledgment of gameplay effects.
class NativePhaseEventQueries final {
public:
    static PhaseQueryStatus Bind(std::shared_ptr<runtime::native::NativeRuntime>,
        std::shared_ptr<NativePhaseEventQueries>&);
    static std::optional<PhaseIntegerQuerySpec> Find(std::string_view registered_identifier) noexcept;
    NativePhaseEventQueries(const NativePhaseEventQueries&)=delete;
    NativePhaseEventQueries& operator=(const NativePhaseEventQueries&)=delete;
    PhaseQueryStatus Validate() const noexcept;
    PhaseQueryStatus Query(PhaseIntegerQuery, std::span<const std::int32_t>, std::int32_t& out) const noexcept;
    void Retire() noexcept { retired_=true; }
    const std::shared_ptr<runtime::native::NativeRuntime>& runtime() const noexcept {return runtime_;}
private:
    NativePhaseEventQueries()=default;
    std::shared_ptr<runtime::native::NativeRuntime> runtime_;
    std::shared_ptr<const runtime::native::NativePlayerEventState> player_;
    runtime::native::TacticalPhaseContext phase_;
    bool retired_{};
};
}
