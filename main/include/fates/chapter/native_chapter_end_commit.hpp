#pragma once
#include <cstdint>
#include <string_view>

namespace fates::chapter::native {
constexpr int kChapterEndUnitPoolSlots = 250;
constexpr int kChapterEndTransporterSlots = 500;
constexpr int kChapterRecordCapacity = 64;
constexpr int kNonRecordCompletionCounterCap = 9999;
constexpr std::uint8_t kLostForceType = 4;
constexpr std::uint8_t kFreePoolForceType = 9;
constexpr std::uint32_t kChapterEndTransientFlagClearMask = 0x000001C0u;
constexpr std::uint32_t kDeadItemDisposalStateBit = 0x00000080u;
constexpr std::uint32_t kLostUnitReleaseStateBit = 0x10000000u;

std::string_view LeaderPrivateSkillIdShiftJis();
std::string_view ChapterLimitedItemSkillIdShiftJis();

bool ShouldProcessChapterEndUnit(std::uint8_t force_type);
bool ShouldPurgeChapterLimitedItem(bool item_present, bool has_chapter_limited_item_skill);
std::uint32_t ClearChapterEndTransientFlags(std::uint32_t flags);
bool ShouldDisposeLostUnitItems(std::uint8_t force_type, std::uint32_t flags);
bool ShouldReleaseLostUnitToFreePool(std::uint8_t force_type, std::uint32_t flags);

bool ShouldCommitPersistentChapterEnd(bool complete_mode, bool map_missing, bool special_dispos_sortie_mode);

enum class GameOverChapterEndExit : std::uint8_t { Return, JumpGlobalProcessLabel9 };
GameOverChapterEndExit ResolveGameOverChapterEndExit(bool complete_mode, bool global_flag_0x400, bool has_global_process);

struct ChapterRecordDecision {
    bool append{};
    std::uint8_t chapter_id{};
    std::uint16_t turn{};
    std::int32_t elapsed{};
    std::uint8_t resulting_count{};
};
ChapterRecordDecision ResolveChapterRecordAppend(bool chapter_record_mode, std::uint8_t existing_count,
    std::uint8_t chapter_id, std::uint16_t turn, std::int32_t total_counter, std::int32_t chapter_start_counter);

struct NonRecordCompletionDecision {
    bool update_contents_earliest_turn{};
    std::uint16_t content_index{};
    std::uint8_t clamped_turn{};
    bool increment_nonrecord_counter{};
    int resulting_nonrecord_counter{};
};
NonRecordCompletionDecision ResolveNonRecordCompletion(std::uint32_t content_id, bool encounter_context,
    int existing_nonrecord_counter, int current_turn);

struct StoryProgressMaxima {
    int offspring_seal_level_max{};
    std::uint32_t raw_chapter_0x15_max{};
};
StoryProgressMaxima UpdateStoryProgressMaxima(std::uint8_t chapter_type, int existing_offspring_seal_max,
    std::uint32_t existing_raw_0x15_max, std::int8_t chapter_offspring_seal_level_retail_signed,
    std::uint8_t chapter_raw_0x15);

bool ShouldIncrementCastlePostChapterCounter(std::uint32_t game_user_flags, bool has_castle_nested_state);
std::uint8_t IncrementCastlePostChapterCounter(std::uint8_t current_value);
} // namespace fates::chapter::native
