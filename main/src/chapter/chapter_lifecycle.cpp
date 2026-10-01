#include "fates/chapter/chapter_run_state.hpp"
namespace fates::chapter {
// Exact retail identity is source-owned; unresolved object layouts stay behind RunStateRuntime.
RunStateWord MainSequence__anonymous_namespace__ProcSequence__ChapterComplete(RunStateRuntime& rt,const RunStateWord* a,std::size_t n){return InvokeRunState(rt,0x001B64D8u,a,n);}
RunStateWord MainSequence__anonymous_namespace__ProcSequence__ChapterSaveAfter(RunStateRuntime&,const RunStateWord*,std::size_t){return 0; // retail 0x001B6698 is an exact no-op
}
RunStateWord MainSequence__anonymous_namespace__ProcSequence__ChapterSaveBefore(RunStateRuntime& rt,const RunStateWord* a,std::size_t n){return InvokeRunState(rt,0x001B669Cu,a,n);}
RunStateWord ChapterSequence__anonymous_namespace__ProcSequence__ChapterEndImpl(RunStateRuntime& rt,const RunStateWord* a,std::size_t n){return InvokeRunState(rt,0x001B6FBCu,a,n);}
RunStateWord ChapterSequence__anonymous_namespace__ProcSequence__BackupLoad(RunStateRuntime& rt,const RunStateWord* a,std::size_t n){return InvokeRunState(rt,0x001DA0E8u,a,n);}
RunStateWord ChapterSequence__anonymous_namespace__ProcSequence__MapSetting(RunStateRuntime& rt,const RunStateWord* a,std::size_t n){return InvokeRunState(rt,0x001DA110u,a,n);}
RunStateWord ChapterSequence__anonymous_namespace__ProcSequence__Persistent(RunStateRuntime& rt,const RunStateWord* a,std::size_t n){return InvokeRunState(rt,0x001DA1D8u,a,n);}
RunStateWord ChapterSequence__anonymous_namespace__ProcSequence__ScriptFree(RunStateRuntime& rt,const RunStateWord* a,std::size_t n){return InvokeRunState(rt,0x001DA1F4u,a,n);}
RunStateWord ChapterSequence__anonymous_namespace__ProcSequence__ScriptLoad(RunStateRuntime& rt,const RunStateWord* a,std::size_t n){return InvokeRunState(rt,0x001DA2DCu,a,n);}
RunStateWord ChapterSequence__anonymous_namespace__ProcSequence__JumpRestart(RunStateRuntime& rt,const RunStateWord* a,std::size_t n){return InvokeRunState(rt,0x001DA390u,a,n);}
RunStateWord ChapterSequence__anonymous_namespace__ProcSequence__SetupBranch(RunStateRuntime& rt,const RunStateWord* a,std::size_t n){return InvokeRunState(rt,0x001DA394u,a,n);}
RunStateWord ChapterSequence__anonymous_namespace__ProcSequence__BackupBranch(RunStateRuntime& rt,const RunStateWord* a,std::size_t n){return InvokeRunState(rt,0x001DA460u,a,n);}
RunStateWord ChapterSequence__anonymous_namespace__ProcSequence__MapEndBranch(RunStateRuntime& rt,const RunStateWord* a,std::size_t n){return InvokeRunState(rt,0x001DA488u,a,n);}
RunStateWord ChapterSequence__anonymous_namespace__ProcSequence__SortieBranch(RunStateRuntime& rt,const RunStateWord* a,std::size_t n){return InvokeRunState(rt,0x001DA58Cu,a,n);}
RunStateWord ChapterSequence__anonymous_namespace__ProcSequence__BackupRelease(RunStateRuntime& rt,const RunStateWord* a,std::size_t n){return InvokeRunState(rt,0x001DA5C0u,a,n);}
RunStateWord ChapterSequence__anonymous_namespace__ProcSequence__SpecialDispos(RunStateRuntime& rt,const RunStateWord* a,std::size_t n){return InvokeRunState(rt,0x001DA5D0u,a,n);}
RunStateWord ChapterSequence__anonymous_namespace__ProcSequence__DeleteTemporary(RunStateRuntime& rt,const RunStateWord* a,std::size_t n){return InvokeRunState(rt,0x001DB1A0u,a,n);}
RunStateWord ChapterSequence__anonymous_namespace__ProcSequence__SortieEndForSkip(RunStateRuntime& rt,const RunStateWord* a,std::size_t n){return InvokeRunState(rt,0x001DB1DCu,a,n);}
RunStateWord ChapterSequence__anonymous_namespace__ProcSequence__ContentsSoundFree(RunStateRuntime& rt,const RunStateWord* a,std::size_t n){return InvokeRunState(rt,0x001DB1F4u,a,n);}
RunStateWord ChapterSequence__anonymous_namespace__ProcSequence__ContentsSoundLoad(RunStateRuntime& rt,const RunStateWord* a,std::size_t n){return InvokeRunState(rt,0x001DB1F8u,a,n);}
RunStateWord ChapterSequence__anonymous_namespace__ProcSequence__MapSequenceCreate(RunStateRuntime& rt,const RunStateWord* a,std::size_t n){return InvokeRunState(rt,0x001DB1FCu,a,n);}
RunStateWord ChapterSequence__anonymous_namespace__ProcSequence__ChapterEndForComplete(RunStateRuntime& rt,const RunStateWord* a,std::size_t n){return InvokeRunState(rt,0x001DB40Cu,a,n);}
RunStateWord ChapterSequence__anonymous_namespace__ProcSequence__ChapterEndForGameOver(RunStateRuntime& rt,const RunStateWord* a,std::size_t n){return InvokeRunState(rt,0x001DB414u,a,n);}
RunStateWord ChapterSequence__anonymous_namespace__ProcSequence__SetupFromGameUserData(RunStateRuntime& rt,const RunStateWord* a,std::size_t n){return InvokeRunState(rt,0x001DB41Cu,a,n);}
RunStateWord ChapterSequence__anonymous_namespace__ProcSequence__VersusDisposNormalOne(RunStateRuntime& rt,const RunStateWord* a,std::size_t n){return InvokeRunState(rt,0x001DB4D8u,a,n);}
RunStateWord ChapterSequence__anonymous_namespace__ProcSequence__SetupFromGameBackupHeader(RunStateRuntime& rt,const RunStateWord* a,std::size_t n){return InvokeRunState(rt,0x001DB5B4u,a,n);}
RunStateWord ChapterSequence__anonymous_namespace__ProcSequence__AddContentNormalizeMessage(RunStateRuntime& rt,const RunStateWord* a,std::size_t n){return InvokeRunState(rt,0x001DB6A0u,a,n);}
RunStateWord ChapterSequence__anonymous_namespace__ProcSequence__UnitPoolSortStoreForOpening(RunStateRuntime& rt,const RunStateWord* a,std::size_t n){return InvokeRunState(rt,0x001DB714u,a,n);}
RunStateWord ChapterSequence__anonymous_namespace__ProcSequence__SetSharedTextureFromUserData(RunStateRuntime& rt,const RunStateWord* a,std::size_t n){return InvokeRunState(rt,0x001DB730u,a,n);}
RunStateWord ChapterSequence__anonymous_namespace__ProcSequence__RandomInitializeBeforeMapLoad(RunStateRuntime& rt,const RunStateWord* a,std::size_t n){return InvokeRunState(rt,0x001DB76Cu,a,n);}
RunStateWord ChapterSequence__anonymous_namespace__ProcSequence__UnitPoolSortRestoreForOpening(RunStateRuntime& rt,const RunStateWord* a,std::size_t n){return InvokeRunState(rt,0x001DB7A8u,a,n);}
RunStateWord ChapterSequence__anonymous_namespace__ProcSequence__CastleAfterBattleSequenceForComplete(RunStateRuntime& rt,const RunStateWord* a,std::size_t n){return InvokeRunState(rt,0x001DB7D0u,a,n);}
RunStateWord ChapterSequence__anonymous_namespace__ProcSequence__CastleAfterBattleSequenceForGameOver(RunStateRuntime& rt,const RunStateWord* a,std::size_t n){return InvokeRunState(rt,0x001DB860u,a,n);}
RunStateWord ChapterSequence__anonymous_namespace__ProcSequence__MapEnd(RunStateRuntime& rt,const RunStateWord* a,std::size_t n){return InvokeRunState(rt,0x001DB8F0u,a,n);}
RunStateWord ChapterSequence__anonymous_namespace__ProcSequence__MessFree(RunStateRuntime& rt,const RunStateWord* a,std::size_t n){return InvokeRunState(rt,0x001DB980u,a,n);}
RunStateWord ChapterSequence__anonymous_namespace__ProcSequence__MessLoad(RunStateRuntime& rt,const RunStateWord* a,std::size_t n){return InvokeRunState(rt,0x001DB9E4u,a,n);}
RunStateWord ChapterSequence__anonymous_namespace__ProcSequence__ProcSequence_2(RunStateRuntime& rt,const RunStateWord* a,std::size_t n){return InvokeRunState(rt,0x001DBA74u,a,n);}
RunStateWord ChapterSequence__Create(RunStateRuntime& rt,const RunStateWord* a,std::size_t n){return InvokeRunState(rt,0x001DBCE8u,a,n);}
RunStateWord ChapterSequence__anonymous_namespace__ProcSequence__SortieEnd(RunStateRuntime& rt,const RunStateWord* a,std::size_t n){return InvokeRunState(rt,0x001E7D94u,a,n);}
RunStateWord MainSequence__anonymous_namespace__ProcSequence__ChapterSaveAfterPurchaseRouteFailed(RunStateRuntime& rt,const RunStateWord* a,std::size_t n){return InvokeRunState(rt,0x0021510Cu,a,n);}
RunStateWord MainSequence__anonymous_namespace__ProcSequence__ChapterSave(RunStateRuntime& rt,const RunStateWord* a,std::size_t n){return InvokeRunState(rt,0x00215354u,a,n);}
RunStateWord ChapterSequence__anonymous_namespace__ProcSequence__AddContentNormalizeUnitEscape(RunStateRuntime& rt,const RunStateWord* a,std::size_t n){return InvokeRunState(rt,0x00419BBCu,a,n);}
RunStateWord ChapterSequence__anonymous_namespace__ProcSequence__MapSettingEvent(RunStateRuntime& rt,const RunStateWord* a,std::size_t n){return InvokeRunState(rt,0x004222E4u,a,n);}
RunStateWord ChapterSequence__anonymous_namespace__ProcSequence__IsSortieSkip(RunStateRuntime& rt,const RunStateWord* a,std::size_t n){return InvokeRunState(rt,0x0050B904u,a,n);}
RunStateWord ChapterSequence__anonymous_namespace__ProcSequence__IsMapLoadSkip(RunStateRuntime& rt,const RunStateWord* a,std::size_t n){return InvokeRunState(rt,0x0050B970u,a,n);}
RunStateWord ChapterSequence__anonymous_namespace__ProcSequence__IsOpeningSkip(RunStateRuntime& rt,const RunStateWord* a,std::size_t n){return InvokeRunState(rt,0x0050B98Cu,a,n);}
}
