#pragma once
#include "fates/event/native_commands.hpp"

#include <cstddef>
#include <cstdint>
#include <string_view>

namespace fates::event::native {

// Typed host-facing contract for the first high-value CMVM gameplay slice.
// Exact retail ev::* identity remains in the generic registry/evidence.  A
// connected callback receives semantic arguments rather than NativeWord*.
// Missing typed callbacks deliberately fall through to NativeCommandRuntime::invoke.
struct TypedEventRuntime {
    void* user = nullptr;

    struct CoreCallbacks {
        bool (*flag_get)(void*, const char*) = nullptr;
        void (*flag_set)(void*, const char*) = nullptr;
        void (*flag_clear)(void*, const char*) = nullptr;
        std::int32_t (*variable_get)(void*, const char*) = nullptr;
        void (*variable_set)(void*, const char*, std::int32_t) = nullptr;
        void (*variable_add)(void*, const char*, std::int32_t) = nullptr;
        std::int32_t (*random_get_game)(void*) = nullptr;
        std::int32_t (*random_get_system)(void*) = nullptr;
    } core;

    struct ChapterCallbacks {
        const char* (*get_name)(void*) = nullptr;
        bool (*is_cid)(void*, const char*) = nullptr;
        std::uint8_t (*get_win_lose_result)(void*) = nullptr;
        void (*set_win)(void*) = nullptr;
        void (*set_lose)(void*) = nullptr;
        void (*set_skip_sortie)(void*, bool) = nullptr;
        void (*set_prohibit_warp)(void*, bool) = nullptr;
        void (*set_exp_mode)(void*, std::uint8_t) = nullptr;
        void (*set_invalid_phoenix)(void*, bool) = nullptr;
        void (*set_invalid_gain_dragon_vein)(void*, bool) = nullptr;
        void (*set_win_rule_limit_turn)(void*, std::uint16_t) = nullptr;
        void (*set_win_rule_enemy_number_le)(void*, std::uint8_t) = nullptr;
    } chapter;

    struct DisposCallbacks {
        bool (*load)(void*, const char*) = nullptr;
        void (*free)(void*) = nullptr;
        void (*run_group)(void*, const char*, std::int32_t) = nullptr;
        bool (*is_wait)(void*) = nullptr;
    } dispos;

    struct ForceCallbacks {
        std::uint8_t (*get_active)(void*) = nullptr;
        std::int32_t (*unit_get_count)(void*, std::int32_t) = nullptr;
    } force;

    struct UnitCallbacks {
        std::int32_t (*get_by_pid)(void*, const char*) = nullptr;
        bool (*is_pid)(void*, std::int32_t, const char*) = nullptr;
        std::int32_t (*get_by_position)(void*, std::int32_t, std::int32_t) = nullptr;
        std::int32_t (*get_force)(void*, std::int32_t) = nullptr;
        std::int32_t (*get_x)(void*, std::int32_t) = nullptr;
        std::int32_t (*get_y)(void*, std::int32_t) = nullptr;
        void (*set_position)(void*, std::int32_t, std::int32_t, std::int32_t) = nullptr;
        void (*move_position)(void*, std::int32_t, std::int32_t, std::int32_t, std::uint32_t) = nullptr;
        bool (*is_move_wait)(void*) = nullptr;
        bool (*is_alive)(void*, std::int32_t) = nullptr;
        std::int32_t (*get_hp)(void*, std::int32_t) = nullptr;
        std::int32_t (*get_mhp)(void*, std::int32_t) = nullptr;
        void (*set_hp)(void*, std::int32_t, std::int32_t) = nullptr;
        std::int32_t (*get_move_power)(void*, std::int32_t, bool) = nullptr;
        void (*set_move_power)(void*, std::int32_t, std::int32_t) = nullptr;
    } unit;

    struct AiCallbacks {
        void (*set_sequence)(void*, std::int32_t, std::uint8_t, const char*, const char*) = nullptr;
        std::uint32_t (*test_flag)(void*, std::int32_t, std::uint32_t) = nullptr;
        void (*set_flag)(void*, std::int32_t, std::uint32_t) = nullptr;
        void (*clear_flag)(void*, std::int32_t, std::uint32_t) = nullptr;
        void (*set_active)(void*, std::int32_t, std::uint8_t) = nullptr;
        std::uint8_t (*get_active)(void*, std::int32_t) = nullptr;
        void (*set_move_limit)(void*, std::int32_t, std::uint8_t, std::uint8_t, std::uint8_t, std::uint8_t, std::uint8_t) = nullptr;
        void (*set_priority)(void*, std::int32_t, std::uint8_t) = nullptr;
    } ai;
};

enum class TypedCommandFamily : std::uint8_t { Core, Chapter, Dispos, Force, Unit, Ai };
struct TypedCommandSpec {
    const char* command;
    std::uint32_t retail_address;
    TypedCommandFamily family;
    std::uint8_t argument_count;
};

const TypedCommandSpec* GetTypedCommandSpecs();
std::size_t GetTypedCommandSpecCount();
const TypedCommandSpec* FindTypedCommandSpec(std::string_view command);

// Returns true only when the command was recognized, had the exact typed
// argument count, and a corresponding typed callback was connected.  false
// means the caller must preserve compatibility by invoking the generic bridge.
bool TryInvokeTyped(
    TypedEventRuntime& runtime,
    std::string_view command,
    ::cmvm::CmContext* context,
    const NativeWord* args,
    std::size_t argc,
    NativeWord& result
);

namespace policy {
inline constexpr std::int32_t kMinScriptUnitHandle = 1;
inline constexpr std::int32_t kMaxScriptUnitHandle = 250;
inline constexpr std::uint32_t kUnitDeadOrUnavailableMask = 0x18u;
inline constexpr std::uint32_t kChapterSkipSortieFlag = 0x02000000u;
inline constexpr std::uint32_t kChapterProhibitWarpFlag = 0x08000000u;
inline constexpr std::uint32_t kChapterInvalidPhoenixFlag = 0x00080000u;
inline constexpr std::uint32_t kChapterInvalidGainDragonVeinFlag = 0x00100000u;
inline constexpr std::uint8_t kRetailWinLoseResultCode = 9u;

constexpr bool IsScriptUnitHandle(std::int32_t id) {
    return id >= kMinScriptUnitHandle && id <= kMaxScriptUnitHandle;
}
constexpr std::uint32_t SetMask(std::uint32_t value, std::uint32_t mask) { return value | mask; }
constexpr std::uint32_t ClearMask(std::uint32_t value, std::uint32_t mask) { return value & ~mask; }
constexpr std::uint32_t TestMask(std::uint32_t value, std::uint32_t mask) { return value & mask; }
constexpr std::uint32_t ToggleMask(std::uint32_t value, std::uint32_t mask, bool enabled) {
    return enabled ? SetMask(value, mask) : ClearMask(value, mask);
}
constexpr std::int32_t ClampRequestedHp(std::int32_t requested, std::int32_t max_hp) {
    std::int32_t value = requested < 1 ? 1 : requested;
    return value > max_hp ? max_hp : value;
}
constexpr std::int8_t EncodeMovePowerAdjustment(std::int32_t requested, std::int32_t job_base_move) {
    return static_cast<std::int8_t>(static_cast<std::uint32_t>(requested) - static_cast<std::uint32_t>(job_base_move));
}
} // namespace policy

} // namespace fates::event::native
