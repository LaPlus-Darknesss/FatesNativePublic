#pragma once
#include <cstdint>
#include <optional>
#include <span>
#include <vector>
namespace fates::runtime::native {
struct NativeRuntime;
// Explicit native ownership of GameUserData+0x2D. Unknown is not Normal.
// The campaign remains the single owner of the GameUser flag word.
struct NativeGameUserDifficultyState {
    bool bound{};
    std::uint8_t value{};
    std::uint64_t revision{};
    bool operator==(const NativeGameUserDifficultyState&) const=default;
};
struct GameUserDifficultySnapshot {std::uint8_t value{};};
enum class GameUserSettingsStatus : std::uint8_t {Ok,Unbound,AlreadyBound,InvalidValue,Disabled,ForceTransferRequired,RevisionExhausted};
// Carried typed state, not a save-container decoder or a whole GameUser reset.
GameUserSettingsStatus RestoreGameUserDifficulty(NativeRuntime&,GameUserDifficultySnapshot) noexcept;
GameUserSettingsStatus InvalidateGameUserDifficulty(NativeRuntime&) noexcept;
bool GameUserDifficultyMatches(const NativeRuntime&,std::uint8_t scoped) noexcept;
std::optional<std::uint8_t> CurrentGameUserDifficulty(const NativeRuntime&) noexcept;
std::uint32_t DifficultyChoiceAttributesExact(std::uint8_t current,std::uint8_t pending,std::uint8_t choice) noexcept;
std::uint32_t ModeChoiceAttributesExact(std::uint32_t flags,std::uint8_t pending_difficulty,std::uint8_t pending_mode,std::uint8_t choice) noexcept;
struct GameUserPendingChoice {std::uint8_t difficulty{},mode{};bool operator==(const GameUserPendingChoice&) const=default;};
struct GameUserChoiceResult {GameUserPendingChoice pending{};std::uint32_t result{};std::int32_t jump{-1};};
// ACall trusts its stored disabled bit. Hidden bit 4 is a separate UI concern.
GameUserChoiceResult SelectGameUserChoiceExact(GameUserPendingChoice,std::uint32_t attributes,std::uint8_t choice,bool mode) noexcept;
struct GameUserSettingsChangePlan {
    std::uint32_t flags{};
    std::uint8_t difficulty{};
    bool transfer_force4_to3{};
    std::vector<std::uint32_t> force4_flags{};
    std::uint32_t return_code{0x108};
    std::int32_t jump{3}; // The jump is requested before Force 4 edits/transfer.
};
// Complete ConfirmYes state projection with ordered Force 4 flags supplied.
// Force::Transfer internals and ProcInst::Jump remain explicit effects.
GameUserSettingsChangePlan PlanGameUserSettingsChangeExact(std::uint32_t current_flags,
    GameUserPendingChoice,std::span<const std::uint32_t> force4_flags={});
// Stages settings before Force4 revival, so healing sees the new difficulty.
// Unresolved transfer lifetimes retain settings, units, revisions and RNG.
// This gameplay commit does not execute the UI procedure's jump callback.
GameUserSettingsStatus ChangeGameUserSettings(NativeRuntime&,GameUserPendingChoice);
}
