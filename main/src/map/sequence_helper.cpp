#include "fates/map/sequence_helper.hpp"
#include "fates/detail/map_battle_resolution_runtime.hpp"
namespace map::SequenceHelper {
bool DangerTick(){
    // PROVEN: when danger-overlay control is enabled and at least one non-allied
    // Unit exists, retail toggles the global danger bit, refreshes the danger
    // image and plays the matching on/off sound.
    return fates::decomp_detail::MapBattleResolutionValue<bool>("SequenceHelper.DangerTick");
}
void MoveCursor(ProcInst* p,int x,int y){fates::decomp_detail::MapBattleResolutionCall("SequenceHelper.MoveCursor",p,x,y);}
void CheerFromMind(ProcInst* p){fates::decomp_detail::MapBattleResolutionCall("SequenceHelper.CheerFromMind",p);}
bool DangerOneTick(){
    // PROVEN: toggles the cursor-selected enemy's per-Unit danger flag (and its
    // clone where applicable) or clears the relevant enemy-force selection.
    return fates::decomp_detail::MapBattleResolutionValue<bool>("SequenceHelper.DangerOneTick");
}
bool HpEffectIsWait(){
    // PROVEN: true while either retail HP-heal or HP-damage effect process exists.
    return fates::decomp_detail::MapBattleResolutionValue<bool>("SequenceHelper.HpEffectIsWait");
}
bool GetCursorTurnFirst(int forceType,int* x,int* y){return fates::decomp_detail::MapBattleResolutionValue<bool>("SequenceHelper.GetCursorTurnFirst",forceType,x,y);}
void FreeCursorSetMoveImage(const Unit* u){fates::decomp_detail::MapBattleResolutionCall("SequenceHelper.FreeCursorSetMoveImage",u);}
void CheerFromCapabilityIndex(ProcInst* p,Unit* u,int i){fates::decomp_detail::MapBattleResolutionCall("SequenceHelper.CheerFromCapabilityIndex",p,u,i);}
namespace detail {
void WaitCursorProcess::Tick(){fates::decomp_detail::MapBattleResolutionCall("SequenceHelper.WaitCursor.Tick");}
void HpEffectCallbackProcess::Callback(ProcInst* p){fates::decomp_detail::MapBattleResolutionCall("SequenceHelper.HpEffectCallback",p);}
void CheerImpl(ProcInst* p,Unit* a,Unit* b,int i,bool partner){fates::decomp_detail::MapBattleResolutionCall("SequenceHelper.CheerImpl",p,a,b,i,partner);}
}
} // namespace map::SequenceHelper
