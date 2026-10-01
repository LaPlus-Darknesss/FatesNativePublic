#pragma once
#include <cstdint>

namespace fates::chapter::native {

// Portable representation of the two retail Situation outcome flags plus the
// shared raw WinLoseResult byte. The raw byte is intentionally not promoted
// to a guessed player-facing enum name.
struct SituationOutcomeState {
    bool complete_flag{};
    bool game_over_flag{};
    std::uint8_t raw_result{};
};

// Retail SetComplete/SetGameOver are separately one-shot. A second call to the
// same writer is ignored; the other writer may still set its independent flag
// and overwrite the shared raw_result byte.
bool LatchComplete(SituationOutcomeState& state, std::uint8_t raw_result);
bool LatchGameOver(SituationOutcomeState& state, std::uint8_t raw_result);

constexpr bool IsComplete(const SituationOutcomeState& state) { return state.complete_flag; }
constexpr bool IsGameOver(const SituationOutcomeState& state) { return state.game_over_flag; }

enum class MapSequenceEndRoute : std::uint8_t {
    Continue = 0,
    CompleteLabel6 = 6,
    GameOverLabel7 = 7,
};

// Both map::Sequence and SequenceHuman test GameOver before Complete.
MapSequenceEndRoute ResolveMapSequenceGameEnd(const SituationOutcomeState& state);

struct HumanSequenceGameEndDecision {
    MapSequenceEndRoute map_route{MapSequenceEndRoute::Continue};
    bool jump_local{};
    std::uint8_t local_label{}; // 0x29 when an ending route is selected.
};
HumanSequenceGameEndDecision ResolveHumanSequenceGameEnd(const SituationOutcomeState& state);

struct MapCompleteCleanupContract {
    bool resolve_chapter_reliance{};
    bool reset_units_end_of_map{};
    bool restore_lost_units_for_casual_or_phoenix{};
    bool reset_tactical_ui{};
    bool update_global_viewer_when_not_versus{};
    std::uint8_t rng_draws{};
};
MapCompleteCleanupContract RetailMapCompleteCleanupContract();

enum class ChapterMapEndRoute : std::uint8_t {
    CompleteLabel7 = 7,
    GameOverLabel8 = 8,
    NormalLabel9 = 9,
    SpecialStateLabel13 = 13,
};

struct ChapterMapEndDecision {
    ChapterMapEndRoute route{ChapterMapEndRoute::NormalLabel9};
    bool map_active_after{}; // retail always clears map-active before branching.
    bool jump_main_sequence{};
    std::uint8_t main_sequence_label{}; // 5 only for raw sequence state 4.
    bool sync_current_chapter_from_spot{};
    bool clear_transition_mask{};
};

// raw_sequence_state==4 has precedence over both outcome flags. Otherwise
// GameOver has precedence over Complete, then the normal path is label 9.
ChapterMapEndDecision ResolveChapterMapEnd(std::uint8_t raw_sequence_state,
                                           const SituationOutcomeState& state);

enum class ChapterEndDispatch : std::uint8_t {
    None,
    Complete,
    GameOver,
};

// Named ChapterSequence handlers dispatch to ChapterEndImpl(true/false).
ChapterEndDispatch ResolveChapterEndDispatch(ChapterMapEndRoute route);
bool ChapterEndImplMode(ChapterEndDispatch dispatch, bool& complete_mode);

} // namespace fates::chapter::native
