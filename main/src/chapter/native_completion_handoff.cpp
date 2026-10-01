#include "fates/chapter/native_completion_handoff.hpp"

namespace fates::chapter::native {

bool LatchComplete(SituationOutcomeState& state, std::uint8_t raw_result) {
    if (state.complete_flag) return false;
    state.complete_flag = true;
    state.raw_result = raw_result;
    return true;
}

bool LatchGameOver(SituationOutcomeState& state, std::uint8_t raw_result) {
    if (state.game_over_flag) return false;
    state.game_over_flag = true;
    state.raw_result = raw_result;
    return true;
}

MapSequenceEndRoute ResolveMapSequenceGameEnd(const SituationOutcomeState& state) {
    if (state.game_over_flag) return MapSequenceEndRoute::GameOverLabel7;
    if (state.complete_flag) return MapSequenceEndRoute::CompleteLabel6;
    return MapSequenceEndRoute::Continue;
}

HumanSequenceGameEndDecision ResolveHumanSequenceGameEnd(const SituationOutcomeState& state) {
    HumanSequenceGameEndDecision out{};
    out.map_route = ResolveMapSequenceGameEnd(state);
    if (out.map_route != MapSequenceEndRoute::Continue) {
        out.jump_local = true;
        out.local_label = 0x29;
    }
    return out;
}

MapCompleteCleanupContract RetailMapCompleteCleanupContract() {
    return MapCompleteCleanupContract{
        true,  // chapter-reliance resolution / record
        true,  // Unit::ResetEndOfMap over the active set
        true,  // casual/phoenix lost-unit return
        true,  // cursor/panel/danger/terrain/tutorial cleanup
        true,  // global viewer update when not versus
        0      // no RNG calls in retail map::Sequence::Complete
    };
}

ChapterMapEndDecision ResolveChapterMapEnd(std::uint8_t raw_sequence_state,
                                           const SituationOutcomeState& state) {
    ChapterMapEndDecision out{};
    out.map_active_after = false;
    if (raw_sequence_state == 4) {
        out.route = ChapterMapEndRoute::SpecialStateLabel13;
        out.jump_main_sequence = true;
        out.main_sequence_label = 5;
        out.sync_current_chapter_from_spot = true;
        out.clear_transition_mask = true;
        return out;
    }
    if (state.game_over_flag) {
        out.route = ChapterMapEndRoute::GameOverLabel8;
        return out;
    }
    if (state.complete_flag) {
        out.route = ChapterMapEndRoute::CompleteLabel7;
        return out;
    }
    out.route = ChapterMapEndRoute::NormalLabel9;
    return out;
}

ChapterEndDispatch ResolveChapterEndDispatch(ChapterMapEndRoute route) {
    if (route == ChapterMapEndRoute::CompleteLabel7) return ChapterEndDispatch::Complete;
    if (route == ChapterMapEndRoute::GameOverLabel8) return ChapterEndDispatch::GameOver;
    return ChapterEndDispatch::None;
}

bool ChapterEndImplMode(ChapterEndDispatch dispatch, bool& complete_mode) {
    if (dispatch == ChapterEndDispatch::Complete) {
        complete_mode = true;
        return true;
    }
    if (dispatch == ChapterEndDispatch::GameOver) {
        complete_mode = false;
        return true;
    }
    return false;
}

} // namespace fates::chapter::native
