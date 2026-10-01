#include "fates/runtime/native_completion_lifecycle.hpp"
#include "fates/chapter/native_chapter_end_commit.hpp"
#include "fates/chapter/native_completion_handoff.hpp"
#include "fates/campaign/native_campaign_continuity.hpp"
#include <algorithm>
#include <limits>

namespace fates::runtime::native {
namespace {
using namespace fates::chapter::native;
using namespace fates::campaign::native;

void CompactUnitItems(UnitState& unit) {
    std::array<ItemState,5> compact{};
    std::size_t out=0;
    for (const auto& item : unit.items) if (item.present) compact[out++]=item;
    unit.items=compact;
}

bool EligibleDeadDisposalItem(const ItemState& item, std::uint32_t game_user_flags) {
    if (!item.present || item.dead_disposal_primary_excluded) return false;
    // Retail ItemDisposalForDead transfers every primary-eligible item when
    // flag 0x8000 is set. With the flag clear, the secondary ItemSkill mask
    // must match or the item is left on the Unit.
    return (game_user_flags & 0x8000u) != 0 || item.dead_disposal_secondary_match;
}

bool CanAddWithoutRetailReplacement(const TransporterState& t, const ItemState& item) {
    for (const auto& slot : t.slots) {
        if (slot.present && slot.stack_key==item.stack_key && slot.quantity < 250) return true;
    }
    for (const auto& slot : t.slots) if (!slot.present) return true;
    return false;
}

bool AddDeadItemWithoutReplacement(TransporterState& t, const ItemState& item) {
    for (auto& slot : t.slots) {
        if (slot.present && slot.stack_key==item.stack_key && slot.quantity < 250) {
            slot.quantity=static_cast<std::uint16_t>(std::min<int>(250,slot.quantity+1));
            return true;
        }
    }
    for (auto& slot : t.slots) {
        if (!slot.present) {
            slot=item;
            slot.quantity=1;
            return true;
        }
    }
    return false;
}

bool PreflightTransporter(const NativeGameState& state) {
    auto t=state.transporter;
    for (const auto& unit : state.units) {
        if (!unit.occupied || !ShouldProcessChapterEndUnit(unit.force_type)) continue;
        if (!ShouldDisposeLostUnitItems(unit.force_type,unit.flags)) continue;
        for (const auto& item : unit.items) {
            if (item.chapter_limited) continue; // purged before dead disposal
            if (!EligibleDeadDisposalItem(item,state.campaign.game_user_flags)) continue;
            if (!CanAddWithoutRetailReplacement(t,item)) return false;
            AddDeadItemWithoutReplacement(t,item);
        }
    }
    return true;
}

void ApplyUnitAndTransporterCleanup(NativeGameState& state,
                                    const CompletionChapterDefinition& chapter,
                                    const CompletionLifecycleContext& context) {
    for (auto& unit : state.units) {
        if (!unit.occupied || !ShouldProcessChapterEndUnit(unit.force_type)) continue;
        for (auto& item : unit.items) {
            if (ShouldPurgeChapterLimitedItem(item.present,item.chapter_limited)) item=ItemState{};
        }
        CompactUnitItems(unit);
        if (ShouldDisposeLostUnitItems(unit.force_type,unit.flags)) {
            unit.dead_record_valid=true;
            unit.dead_record_chapter_id=chapter.index;
            unit.dead_record_context=context.chapter_context_byte;
            for (auto& item : unit.items) {
                if (!EligibleDeadDisposalItem(item,state.campaign.game_user_flags)) continue;
                AddDeadItemWithoutReplacement(state.transporter,item);
                item=ItemState{};
            }
            CompactUnitItems(unit);
            unit.accessories_moved_to_box=true;
        }
        unit.chapter_scratch=0;
        unit.flags=ClearChapterEndTransientFlags(unit.flags);
        unit.leader_private_skill=false;
        if (ShouldReleaseLostUnitToFreePool(unit.force_type,unit.flags)) unit.force_type=kFreePoolForceType;
    }
    for (auto& slot : state.transporter.slots) {
        if (ShouldPurgeChapterLimitedItem(slot.present,slot.chapter_limited)) slot=ItemState{};
    }
}

void RestoreOrdinarySortieOrder(NativeGameState& state) {
    state.restored_sortie_order_unit_slots.clear();
    std::vector<std::uint16_t> to_force3;
    std::vector<std::uint16_t> to_force4;
    for (std::uint16_t i=0;i<state.units.size();++i) {
        auto& u=state.units[i];
        if (!u.occupied || u.force_type==kFreePoolForceType) continue;
        if (u.force_type==0 || u.force_type==3) to_force3.push_back(i);
        else if (u.force_type==4) to_force4.push_back(i);
    }
    auto by_sortie=[&](std::uint16_t a,std::uint16_t b){
        const auto ka=state.units[a].sortie_order_key, kb=state.units[b].sortie_order_key;
        return ka<kb || (ka==kb && a<b);
    };
    std::stable_sort(to_force3.begin(),to_force3.end(),by_sortie);
    std::stable_sort(to_force4.begin(),to_force4.end(),by_sortie);
    for (auto i:to_force3) { state.units[i].force_type=3; state.restored_sortie_order_unit_slots.push_back(i); }
    for (auto i:to_force4) { state.units[i].force_type=4; state.restored_sortie_order_unit_slots.push_back(i); }
}

SpotState* FindSpot(CampaignState& campaign,std::uint8_t chapter_index) {
    for (auto& s:campaign.spots) if (s.present && s.chapter_index==chapter_index) return &s;
    return nullptr;
}

void ApplyPersistentAccounting(NativeGameState& state,
                               const CompletionChapterDefinition& chapter,
                               const CompletionLifecycleContext& context,
                               CompletionLifecycleResult& result) {
    if (!ShouldCommitPersistentChapterEnd(true,context.map_missing,context.special_dispos_sortie_mode)) return;
    const bool record_mode=IsChapterRecord(context.has_versus_config,state.campaign.game_user_flags,chapter.type);
    const auto rd=ResolveChapterRecordAppend(record_mode,state.campaign.chapter_record_count,chapter.index,
        context.current_turn,context.total_counter,context.chapter_start_counter);
    if (rd.append) {
        auto& rec=state.campaign.chapter_records[state.campaign.chapter_record_count];
        rec.valid=true; rec.chapter_id=rd.chapter_id; rec.turn=rd.turn; rec.elapsed=rd.elapsed;
        state.campaign.chapter_record_count=rd.resulting_count;
        result.chapter_record_appended=true;
    } else if (!record_mode) {
        const auto nd=ResolveNonRecordCompletion(state.campaign.content_id,context.encounter_context,
            state.campaign.nonrecord_completion_counter,context.current_turn);
        if (nd.update_contents_earliest_turn && nd.content_index < state.campaign.contents_earliest_turn.size()) {
            auto& v=state.campaign.contents_earliest_turn[nd.content_index];
            v=UpdateEarliestTurnByte(v,nd.content_index,nd.clamped_turn);
        }
        if (nd.increment_nonrecord_counter) state.campaign.nonrecord_completion_counter=nd.resulting_nonrecord_counter;
    }
    const auto maxima=UpdateStoryProgressMaxima(chapter.type,state.campaign.offspring_seal_level_max,
        state.campaign.raw_chapter_0x15_max,chapter.offspring_seal_level_retail_signed,chapter.raw_chapter_0x15);
    state.campaign.offspring_seal_level_max=maxima.offspring_seal_level_max;
    state.campaign.raw_chapter_0x15_max=maxima.raw_chapter_0x15_max;
    if (ShouldIncrementCastlePostChapterCounter(state.campaign.game_user_flags,state.campaign.has_castle_nested_state))
        state.campaign.castle_post_chapter_counter=IncrementCastlePostChapterCounter(state.campaign.castle_post_chapter_counter);
    result.persistent_commit_applied=true;
}

} // namespace

CompletionLifecycleResult ApplyCompletionLifecycle(
    NativeGameState& state,
    const CompletionChapterDefinition& current_chapter,
    const CompletionChapterDefinition* selected_next_chapter,
    const CompletionLifecycleContext& context) {
    CompletionLifecycleResult result{};
    result.map_route=ResolveMapSequenceGameEnd(state.outcome);
    if (result.map_route==MapSequenceEndRoute::Continue) return result;

    const auto chapter_decision=ResolveChapterMapEnd(context.raw_sequence_state,state.outcome);
    result.chapter_route=chapter_decision.route;
    if (chapter_decision.route==ChapterMapEndRoute::SpecialStateLabel13) {
        result.status=CompletionLifecycleStatus::SpecialSequenceRoute;
        return result;
    }
    const auto dispatch=ResolveChapterEndDispatch(chapter_decision.route);
    bool complete_mode=false;
    if (!ChapterEndImplMode(dispatch,complete_mode)) return result;

    // Unsupported subtransactions are detected before mutation.
    if (context.special_dispos_sortie_mode) {
        result.status=CompletionLifecycleStatus::NeedsSpecialSortieRestore;
        return result;
    }
    if (!PreflightTransporter(state)) {
        result.status=CompletionLifecycleStatus::NeedsTransporterReplacementPolicy;
        return result;
    }
    if (complete_mode) {
        const auto preview=ResolveMainSequenceChapterComplete(state.campaign.game_user_flags,current_chapter.type,
            state.campaign.route,current_chapter.next_birthright,current_chapter.next_conquest,current_chapter.next_revelation,
            selected_next_chapter && selected_next_chapter->cid_ending);
        if (preview.update_world_mobs) {
            result.status=CompletionLifecycleStatus::NeedsWorldMobUpdate;
            return result;
        }
    }

    state.map_active=false;
    ApplyUnitAndTransporterCleanup(state,current_chapter,context);
    RestoreOrdinarySortieOrder(state);

    if (!complete_mode) {
        result.status=CompletionLifecycleStatus::GameOverCommitted;
        return result;
    }

    ApplyPersistentAccounting(state,current_chapter,context,result);

    const auto advance=ResolveMainSequenceChapterComplete(state.campaign.game_user_flags,current_chapter.type,
        state.campaign.route,current_chapter.next_birthright,current_chapter.next_conquest,current_chapter.next_revelation,
        selected_next_chapter && selected_next_chapter->cid_ending);
    result.campaign_route=advance.route;
    result.next_chapter_index=advance.next_chapter_index;
    if (advance.update_current_spot) {
        if (auto* current=FindSpot(state.campaign,current_chapter.index)) {
            const auto mark=MarkCompletedSpot(state.campaign.current_level);
            current->state=mark.state; current->stored_level=mark.stored_level;
        }
    }
    if (advance.route==ChapterCompleteRoute::ResetCurrentSpotMobs ||
        advance.route==ChapterCompleteRoute::ReturnNoAdvance ||
        advance.route==ChapterCompleteRoute::JumpLabel2 ||
        advance.route==ChapterCompleteRoute::JumpEndingLabel10) {
        result.status=CompletionLifecycleStatus::CompleteCampaignExit;
        return result;
    }
    if (advance.set_spot_current_to_next && selected_next_chapter) {
        state.campaign.spot_current_chapter_index=advance.next_chapter_index;
        if (auto* next=FindSpot(state.campaign,advance.next_chapter_index)) {
            const auto req=SelectRouteRequirement(selected_next_chapter->requirement_birthright,
                selected_next_chapter->requirement_conquest,selected_next_chapter->requirement_revelation,state.campaign.route);
            if (UnlockOrdinaryRequirementSpot(next->state,req==current_chapter.index,selected_next_chapter->type,0)) next->state=1;
        }
    }

    // ChapterSaveBefore uses the original flags for branch precedence, then
    // synchronizes GameUserData current Chapter from Spot and clears the exact
    // retail transient mask.
    const auto original_flags=state.campaign.game_user_flags;
    state.campaign.current_chapter_index=state.campaign.spot_current_chapter_index;
    state.save.save_chapter_index=state.campaign.current_chapter_index;
    state.campaign.game_user_flags &= ~kChapterSaveBeforeClearMask;
    state.save.chapter_save_before_applied=true;
    state.save.chapter_save_jump_label=ResolveChapterSaveBeforeJump(original_flags);
    result.save_jump_label=state.save.chapter_save_jump_label;
    if (state.save.chapter_save_jump_label>=0) {
        result.status=CompletionLifecycleStatus::CompleteSaveRedirect;
        return result;
    }

    const auto menu=ResolveSaveMenuPolicy(context.purchase_route_failed);
    state.save.save_menu_requested=true;
    state.save.purchase_route_failed=context.purchase_route_failed;
    state.save.menu_byte_138=menu.byte138;
    state.save.menu_byte_139=menu.byte139;
    state.save.menu_flags_50|=menu.flag50Or;
    state.save.backup_write_boundary_exposed=true;
    result.save_menu_requested=true;
    result.backup_write_boundary_exposed=true;
    result.status=CompletionLifecycleStatus::CompleteSaveFacing;
    return result;
}

} // namespace fates::runtime::native
