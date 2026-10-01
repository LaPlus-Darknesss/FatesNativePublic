#include "fates/headless/native_chapter_differential.hpp"

#include "fates/ai/native_ai_semantics.hpp"
#include "fates/battle/native_battle_semantics.hpp"
#include "fates/campaign/native_campaign_continuity.hpp"
// Include the event facade before fates::cmvm is introduced: legacy retail event declarations
// intentionally forward-declare ::cmvm::CmContext while the portable VM lives in fates::cmvm.
#include "fates/event/typed_event_commands.hpp"
#include "fates/cmvm/cmvm.hpp"
#include "fates/detail/cmvm_runtime.hpp"
#include "fates/map/native_deployment_semantics.hpp"
#include "fates/map/native_interaction_semantics.hpp"
#include "fates/map/native_terrain_semantics.hpp"
#include "fates/unit/native_unit_semantics.hpp"

namespace fates::headless::native {
namespace {
std::uint16_t ReadLe16(std::span<const std::byte> bytes, std::size_t off, bool& ok) noexcept {
    if (off + 2 > bytes.size()) { ok = false; return 0; }
    return static_cast<std::uint16_t>(std::to_integer<std::uint8_t>(bytes[off])) |
           static_cast<std::uint16_t>(std::to_integer<std::uint8_t>(bytes[off + 1]) << 8U);
}
std::uint32_t ReadLe32(std::span<const std::byte> bytes, std::size_t off, bool& ok) noexcept {
    if (off + 4 > bytes.size()) { ok = false; return 0; }
    return static_cast<std::uint32_t>(std::to_integer<std::uint8_t>(bytes[off])) |
           (static_cast<std::uint32_t>(std::to_integer<std::uint8_t>(bytes[off + 1])) << 8U) |
           (static_cast<std::uint32_t>(std::to_integer<std::uint8_t>(bytes[off + 2])) << 16U) |
           (static_cast<std::uint32_t>(std::to_integer<std::uint8_t>(bytes[off + 3])) << 24U);
}
} // namespace

bool IsNativeCmvmEntryOpcodeOwned(const std::uint8_t opcode) noexcept {
    return fates::cmvm::is_source_owned_opcode(opcode);
}

CmvmArchiveProbe ProbeCmvmArchive(const std::span<const std::byte> archive) noexcept {
    CmvmArchiveProbe out{};
    out.certified = fates::cmvm::certify_archive(archive.data(), archive.size());
    if (!out.certified || archive.size() < 0x20) return out;
    bool ok = true;
    out.version = ReadLe32(archive, 0x04, ok);
    out.function_table_offset = ReadLe32(archive, 0x1C, ok);
    if (!ok || out.function_table_offset >= archive.size()) return CmvmArchiveProbe{};
    for (std::size_t i = 0; i < kMaxCmvmFunctionsInProbe; ++i) {
        const auto table_off = static_cast<std::size_t>(out.function_table_offset) + i * 4U;
        const auto record_off = ReadLe32(archive, table_off, ok);
        if (!ok || record_off == 0) break;
        if (record_off + 12U > archive.size()) return CmvmArchiveProbe{};
        const auto code_off = ReadLe32(archive, static_cast<std::size_t>(record_off) + 4U, ok);
        if (!ok || code_off >= archive.size()) return CmvmArchiveProbe{};
        auto& fn = out.functions[i];
        fn.record_offset = record_off;
        fn.code_offset = code_off;
        fn.type = std::to_integer<std::uint8_t>(archive[record_off + 8U]);
        fn.argc = std::to_integer<std::uint8_t>(archive[record_off + 9U]);
        fn.local_words = ReadLe16(archive, static_cast<std::size_t>(record_off) + 10U, ok);
        if (!ok) return CmvmArchiveProbe{};
        fn.first_opcode = std::to_integer<std::uint8_t>(archive[code_off]);
        fn.first_opcode_owned = IsNativeCmvmEntryOpcodeOwned(fn.first_opcode);
        ++out.function_count;
        if (fn.first_opcode_owned) ++out.native_enterable_function_count;
    }
    return out;
}

SemanticReadinessSnapshot RunSemanticReadinessProbe(const SemanticReadinessInput& input) noexcept {
    SemanticReadinessSnapshot out{};
    out.encoded_mode_flags = fates::campaign::native::SetModeBits(input.user_flags, input.requested_mode);
    out.decoded_mode = fates::campaign::native::GetModeFromBits(out.encoded_mode_flags);
    out.manhattan_distance = fates::map::native::ManhattanDistance(input.x0, input.y0, input.x1, input.y1);
    out.terrain_cost_stride = fates::map::native::TerrainCostStride(input.terrain_cost_width);
    out.can_use_dragon_vein = fates::map::native::CanDragonVein(
        input.dragon_vein_private_mask, input.unit_category_mask,
        input.dragon_vein_block_flag, input.person_is_download);
    out.attack_item_mask = fates::map::native::BuildAttackItemMask(input.attack_can_equip, input.attack_excluded);
    out.ai_priority_score = fates::ai::native::ResolvePriorityScore(input.ai_priority, input.ai_move_power);
    out.hit = fates::unit::native::ResolveHit(fates::unit::native::HitInputs{
        true, input.job_hit, input.item_hit, input.tech, input.luck, true,
        input.weapon_rank_hit_bonus, input.hit_plus_10, false, false});
    out.clamped_hp = fates::event::native::policy::ClampRequestedHp(input.requested_hp, input.max_hp);
    out.battle_proc_success = fates::battle::native::BattleProcRollSucceeds(
        input.proc_roll, input.proc_probability, input.proc_skill_id);
    return out;
}

} // namespace fates::headless::native
