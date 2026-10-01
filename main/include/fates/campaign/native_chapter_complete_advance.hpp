#pragma once
#include <cstdint>
#include "fates/campaign/native_campaign_continuity.hpp"
namespace fates::campaign::native {
constexpr std::uintptr_t kSystemRngRetailAddress=0x00752B7Cu;
constexpr std::uintptr_t kGameRngRetailAddress=0x00752B8Cu;
constexpr std::uintptr_t kWorldMobRngRetailAddress=0x00752B9Cu;

enum class ChapterCompleteRoute : std::uint8_t { ReturnNoAdvance, ResetCurrentSpotMobs, Advance, JumpLabel2, JumpEndingLabel10 };
struct ChapterCompleteDecision {
 ChapterCompleteRoute route{ChapterCompleteRoute::ReturnNoAdvance};
 bool update_current_spot{};
 bool set_spot_current_to_next{};
 bool update_world_mobs{};
 std::uint8_t next_chapter_index{};
};
ChapterCompleteDecision ResolveMainSequenceChapterComplete(std::uint32_t game_user_flags,
 std::uint8_t current_chapter_type, RouteIndex route,
 std::uint8_t next_birthright, std::uint8_t next_conquest, std::uint8_t next_revelation,
 bool selected_next_is_cid_ending);

struct CompletedSpotMark { std::uint8_t state{2}; std::int16_t stored_level{}; };
CompletedSpotMark MarkCompletedSpot(int current_level);
bool UnlockOrdinaryRequirementSpot(std::uint8_t current_state, bool requirement_satisfied,
 std::uint8_t chapter_type, std::uint16_t married_character);

bool WorldMobImmediateSpawnPassEnabled(RouteIndex route);
int WorldMobImmediateSpawnSuccessCap();
int WorldMobSpawnWeight(int sorted_index);
bool WorldMobCombatConsumesDraw(int mob_count);
MobCombatOutcome ResolveWorldMobCombatRoll(int roll);
bool WorldMobFallbackSpawnAllowed(RouteIndex route, bool any_current_mob_state1, int remaining_weight_sum);
struct MobDisposDrawPlan { bool slot_tie_draw{}; bool lifetime_draw{}; bool job_index_draw{}; bool seed_draw{}; };
MobDisposDrawPlan ResolveMobDisposDrawPlan(bool both_slots_empty, int enumerated_job_count);
} // namespace fates::campaign::native
