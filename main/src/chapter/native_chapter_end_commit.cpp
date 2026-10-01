#include "fates/chapter/native_chapter_end_commit.hpp"
#include <algorithm>

namespace fates::chapter::native {
std::string_view LeaderPrivateSkillIdShiftJis() {
    static constexpr char kId[] = "SPID_\x83\x8A\x81\x5B\x83\x5F\x81\x5B";
    return {kId, sizeof(kId)-1};
}
std::string_view ChapterLimitedItemSkillIdShiftJis() {
    static constexpr char kId[] = "ISID_\x8F\xCD\x8C\xC0\x92\xE8";
    return {kId, sizeof(kId)-1};
}
bool ShouldProcessChapterEndUnit(std::uint8_t force_type) { return force_type != kFreePoolForceType; }
bool ShouldPurgeChapterLimitedItem(bool item_present, bool has_chapter_limited_item_skill) {
    return item_present && has_chapter_limited_item_skill;
}
std::uint32_t ClearChapterEndTransientFlags(std::uint32_t flags) {
    return flags & ~kChapterEndTransientFlagClearMask;
}
bool ShouldDisposeLostUnitItems(std::uint8_t force_type, std::uint32_t flags) {
    return force_type == kLostForceType && (flags & kDeadItemDisposalStateBit) != 0;
}
bool ShouldReleaseLostUnitToFreePool(std::uint8_t force_type, std::uint32_t flags) {
    return force_type == kLostForceType && (flags & kLostUnitReleaseStateBit) != 0;
}
bool ShouldCommitPersistentChapterEnd(bool complete_mode, bool map_missing, bool special_dispos_sortie_mode) {
    return complete_mode && !map_missing && !special_dispos_sortie_mode;
}
GameOverChapterEndExit ResolveGameOverChapterEndExit(bool complete_mode, bool global_flag_0x400, bool has_global_process) {
    if (!complete_mode && !global_flag_0x400 && has_global_process) return GameOverChapterEndExit::JumpGlobalProcessLabel9;
    return GameOverChapterEndExit::Return;
}
ChapterRecordDecision ResolveChapterRecordAppend(bool chapter_record_mode, std::uint8_t existing_count,
    std::uint8_t chapter_id, std::uint16_t turn, std::int32_t total_counter, std::int32_t chapter_start_counter) {
    ChapterRecordDecision out{};
    out.chapter_id=chapter_id; out.turn=turn; out.elapsed=total_counter-chapter_start_counter; out.resulting_count=existing_count;
    if (chapter_record_mode && existing_count < kChapterRecordCapacity) {
        out.append=true; out.resulting_count=static_cast<std::uint8_t>(existing_count+1);
    }
    return out;
}
NonRecordCompletionDecision ResolveNonRecordCompletion(std::uint32_t content_id, bool encounter_context,
    int existing_nonrecord_counter, int current_turn) {
    NonRecordCompletionDecision out{};
    out.resulting_nonrecord_counter=existing_nonrecord_counter;
    if (content_id != 0) {
        out.update_contents_earliest_turn=true;
        out.content_index=static_cast<std::uint16_t>(content_id & 0xFFFFu);
        out.clamped_turn=static_cast<std::uint8_t>(std::clamp(current_turn,0,99));
    } else if (encounter_context) {
        out.increment_nonrecord_counter=true;
        out.resulting_nonrecord_counter=std::min(existing_nonrecord_counter+1,kNonRecordCompletionCounterCap);
    }
    return out;
}
StoryProgressMaxima UpdateStoryProgressMaxima(std::uint8_t chapter_type, int existing_offspring_seal_max,
    std::uint32_t existing_raw_0x15_max, std::int8_t chapter_offspring_seal_level_retail_signed,
    std::uint8_t chapter_raw_0x15) {
    StoryProgressMaxima out{existing_offspring_seal_max,existing_raw_0x15_max};
    if (chapter_type != 0) return out;
    out.offspring_seal_level_max=std::max(existing_offspring_seal_max,static_cast<int>(chapter_offspring_seal_level_retail_signed));
    out.raw_chapter_0x15_max=std::max(existing_raw_0x15_max,static_cast<std::uint32_t>(chapter_raw_0x15));
    return out;
}
bool ShouldIncrementCastlePostChapterCounter(std::uint32_t game_user_flags, bool has_castle_nested_state) {
    return (game_user_flags & 0x1000u) != 0 && has_castle_nested_state;
}
std::uint8_t IncrementCastlePostChapterCounter(std::uint8_t current_value) {
    return static_cast<std::uint8_t>(std::min<int>(static_cast<int>(current_value)+1,5));
}
} // namespace fates::chapter::native
