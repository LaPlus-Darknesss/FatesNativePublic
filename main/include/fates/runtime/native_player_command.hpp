#pragma once
#include "fates/runtime/native_player_action.hpp"
#include "fates/battle/native_battle_transaction.hpp"
#include <cstdint>
#include <optional>

namespace fates::runtime::native {
// Host command protocol, not a reconstruction of retail UI object layouts.
// The phase controller supplies this capability; this module does not advance turns.
struct PlayerControlContext { std::uint8_t active_force{}; bool commands_enabled{true}; };
enum class PlayerCommandKind : std::uint8_t { Wait, Attack };
enum class PlayerCommandStatus : std::uint8_t {
    Ok, NoSelection, ControlDisabled, InvalidUnit, WrongForce, AlreadyActed,
    StaleSelection, InvalidDestination, OccupiedDestination, InvalidTarget,
    UnsupportedPair, UnsupportedEquipmentChange, BattleRejected
};
struct PlayerCommand {
    PlayerCommandKind kind{PlayerCommandKind::Wait};
    std::uint16_t unit_slot{0xFFFFu}, target_slot{0xFFFFu};
    std::int16_t destination_x{}, destination_y{};
    // Zero means keep the currently equipped item. Changing equipment is a
    // separate inventory contract, never an unchecked item-id assignment.
    std::uint16_t requested_item_id{};
};
struct PlayerCommandResult {
    PlayerCommandStatus status{PlayerCommandStatus::InvalidUnit};
    PlayerCommand command{};
    fates::battle::native::BattlePreviewResult forecast{};
    fates::battle::native::BattleTransactionResult battle{};
    bool moved{}, action_committed{};
};
PlayerCommandResult ForecastPlayerCommand(const NativeRuntime&, PlayerControlContext,
                                          const PlayerCommand&);
PlayerCommandResult ExecutePlayerCommand(NativeRuntime&, PlayerControlContext,
                                         const PlayerCommand&);

// Selection/movement intent lives outside NativeGameState. Cancel therefore has
// no gameplay rollback side effects. Commit revalidates the CURRENT state; a
// forecast is not a retained outcome or an RNG reservation.
class PlayerCommandSession {
public:
    PlayerCommandStatus Select(const NativeRuntime&, PlayerControlContext, std::uint16_t);
    PlayerCommandStatus StageMove(const NativeRuntime&, PlayerControlContext, std::int16_t, std::int16_t);
    PlayerCommandResult Forecast(const NativeRuntime&, PlayerControlContext, PlayerCommandKind,
                                 std::uint16_t target=0xFFFFu, std::uint16_t item=0) const;
    PlayerCommandResult Confirm(NativeRuntime&, PlayerControlContext, PlayerCommandKind,
                               std::uint16_t target=0xFFFFu, std::uint16_t item=0);
    void Cancel() noexcept { selection_.reset(); }
    bool has_selection() const noexcept { return selection_.has_value(); }
private:
    struct Selection {
        std::uint16_t slot{},person{},job{},equipped{};
        std::uint8_t force{};
        std::int16_t origin_x{},origin_y{},destination_x{},destination_y{};
        std::uint64_t phase_revision{};
        bool phase_bound{};
        UnitInventoryState inventory{};
        NativeGameUserDifficultyState difficulty{};
        std::uint64_t map_end_revision{};
        std::uint64_t lineage_revision{},capability_revision{};
        std::uint64_t transfer_revision{};
        std::uint64_t slot_generation{};
        std::uint64_t pair_revision{};
    };
    std::optional<Selection> selection_;
    PlayerCommandStatus CheckSelection(const NativeRuntime&, PlayerControlContext) const;
    PlayerCommand MakeCommand(PlayerCommandKind,std::uint16_t,std::uint16_t) const;
};
} // namespace fates::runtime::native
