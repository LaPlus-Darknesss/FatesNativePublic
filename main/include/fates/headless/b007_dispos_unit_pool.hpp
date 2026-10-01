#pragma once

#include "fates/event/typed_event_commands.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace fates::headless {

// Host-safe projection of the FE14 Dispos structures used by the bounded B007
// headless integration slice. Strings remain raw retail Shift-JIS byte strings;
// no retail pointers or 3DS object layouts escape this layer.
struct Fe14DisposSpawnProjection {
    std::string pid;
    std::string job;
    std::uint8_t team{};
    std::uint8_t level{};
    std::int8_t coord1_x{};
    std::int8_t coord1_y{};
    std::int8_t coord2_x{};
    std::int8_t coord2_y{};
    std::uint32_t spawn_flags{};
    std::array<std::string,5> items{};
    std::array<std::uint8_t,5> item_flags{};
    // Paragon declares four metadata bytes per item. The latter three are
    // signed difficulty entries: negative excludes the item; positive requests
    // refinement, which remains outside the ordinary native constructor slice.
    std::array<std::array<std::int8_t,3>,5> item_difficulty_adjustments{};
    std::array<std::string,5> skills{};
    std::uint32_t skill_flags{};
    std::string ai_action, ai_action_param;
    std::string ai_mission, ai_mission_param;
    std::string ai_attack, ai_attack_param;
    std::string ai_movement, ai_movement_param;
    std::uint8_t battle_rate{};
    std::uint8_t priority{};
    std::array<std::uint8_t,3> ai_raw_76_78{};
    std::array<std::uint8_t,5> move_limit{};
    std::uint32_t ai_policy{};
    std::uint8_t runtime_state{}; // Spawn +0x84; zero in ordinary original files.
};

struct Fe14DisposGroupProjection {
    std::string name;
    std::vector<Fe14DisposSpawnProjection> spawns;
};

struct Fe14DisposFileProjection {
    std::vector<Fe14DisposGroupProjection> groups;

    const Fe14DisposGroupProjection* FindGroup(std::string_view name) const;
};

bool ParseFe14DisposProjection(std::span<const std::uint8_t> decoded,
                               Fe14DisposFileProjection& out);

enum class B007PlayerSex : std::uint8_t { Male, Female };

struct HeadlessUnitProjection {
    std::int32_t handle{}; // host-local opaque script handle; no retail numeric-parity claim
    std::string pid;       // raw retail Shift-JIS PID bytes
    bool is_player{};
    bool unique{true};
    bool alive{true};
    std::uint8_t force{};
    bool has_position{};
    std::int8_t x{};
    std::int8_t y{};
};

struct HeadlessMoveIntent {
    std::int32_t handle{};
    std::int32_t x{};
    std::int32_t y{};
    std::uint32_t flags{};
};

enum class HeadlessEventMoveStatus : std::uint8_t { PendingRoute, Active, Complete, Rejected };

// Portable event-movement lifecycle state. Route shape itself remains a resolved
// tactical-map input until the Deploy movement image / Route::Seek RNG path is
// closed. This keeps logical Unit state independent from 3DS Actor/Proc objects.
struct HeadlessEventMoveProjection {
    std::int32_t handle{};
    std::int32_t start_x{};
    std::int32_t start_y{};
    std::int32_t goal_x{};
    std::int32_t goal_y{};
    std::uint32_t script_flags{};
    std::uint32_t primary_deploy_flags{};
    std::uint32_t retry_deploy_flags{};
    std::uint32_t actor_move_flags{};
    bool bounds_gate_bypassed{};
    bool occupied_target_relocation_bypassed{};
    bool destination_reserved{};
    bool route_shape_resolved{};
    int route_rng_draw_count{-1};
    HeadlessEventMoveStatus status{HeadlessEventMoveStatus::PendingRoute};
};

// Bounded event-state projection used to advance the real B007 ending script.
// It intentionally models the exact existing-roster identity path exercised by
// Event11/Event12/Event13. It is not a replacement for full map::Dispos.
class B007UnitPoolProjection {
public:
    static constexpr std::uint32_t kRetailEndingEventDisposMode = 0x804u;
    static constexpr std::uint32_t kRetailB007MoveFlags = 0x2u;
    static constexpr std::uint32_t kRetailEventDeployFlags = 0x2002u;
    static constexpr std::uint32_t kRetailEventDeployRetryFlags = 0x2000u;
    static constexpr std::uint32_t kRetailActorEventMoveBit = 0x80u;

    bool SeedEndingScenario(B007PlayerSex sex);
    void AttachDispos(const Fe14DisposFileProjection* dispos) { dispos_ = dispos; }

    std::int32_t GetByPid(std::string_view pid) const;
    bool IsPid(std::int32_t handle, std::string_view pid) const;
    std::int32_t GetPlayer() const;
    bool PlayerIsMale() const;

    std::int32_t GetForce(std::int32_t handle) const;
    std::int32_t GetX(std::int32_t handle) const;
    std::int32_t GetY(std::int32_t handle) const;

    bool ApplyExistingRosterGroup(std::string_view group_name, std::uint32_t mode);
    void QueueMovePosition(std::int32_t handle, std::int32_t x, std::int32_t y,
                           std::uint32_t flags);
    bool IsMoveWait() const;
    bool CompleteActiveMove();
    bool HasUnresolvedRouteDeterminism() const;

    const std::vector<HeadlessUnitProjection>& Units() const { return units_; }
    const std::vector<HeadlessMoveIntent>& MoveIntents() const { return move_intents_; }
    const std::vector<HeadlessEventMoveProjection>& EventMoves() const { return event_moves_; }

    event::native::TypedEventRuntime MakeTypedRuntime();

private:
    HeadlessUnitProjection* FindHandle(std::int32_t handle);
    const HeadlessUnitProjection* FindHandle(std::int32_t handle) const;
    HeadlessUnitProjection* FindPlayer();
    const HeadlessUnitProjection* FindPlayer() const;
    bool AddExisting(std::string_view pid, bool player);

    const Fe14DisposFileProjection* dispos_{};
    std::vector<HeadlessUnitProjection> units_;
    std::vector<HeadlessMoveIntent> move_intents_;
    std::vector<HeadlessEventMoveProjection> event_moves_;
};

// Raw Shift-JIS identities used by the bounded original-data integration.
std::string_view B007PidPlayerMale();
std::string_view B007PidPlayerFemale();
std::string_view B007PidFelicia();
std::string_view B007PidJakob();
std::string_view B007PidElise();
std::string_view B007PidSilas();

} // namespace fates::headless
