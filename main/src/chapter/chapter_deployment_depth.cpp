#include "fates/chapter/chapter_run_state_depth.hpp"
namespace fates::chapter{
// Exact retail identities are source-owned; unresolved object layouts remain behind DepthRuntime.
DepthWord map__SortiePosition__Get(DepthRuntime&rt,const DepthWord*a,std::size_t n){return InvokeDepth(rt,0x00361600u,a,n);}
DepthWord castle__SortiePositions__GetNum(DepthRuntime&rt,const DepthWord*a,std::size_t n){return InvokeDepth(rt,0x00540D90u,a,n);}
DepthWord castle__Golem__GetNum(DepthRuntime&rt,const DepthWord*a,std::size_t n){return InvokeDepth(rt,0x00490FF4u,a,n);}
DepthWord castle__Lilith__Get(DepthRuntime&rt,const DepthWord*a,std::size_t n){return InvokeDepth(rt,0x00491A38u,a,n);}
DepthWord castle__AfterBattleSequence__CreateBind(DepthRuntime&rt,const DepthWord*a,std::size_t n){return InvokeDepth(rt,0x0045CFC0u,a,n);}
DepthWord map__SortiePosition__SetCastleOffensePosition(DepthRuntime&rt,const DepthWord*a,std::size_t n){return InvokeDepth(rt,0x003614E0u,a,n);}
DepthWord map__SortiePosition__SetCastleDefensePosition(DepthRuntime&rt,const DepthWord*a,std::size_t n){return InvokeDepth(rt,0x003613C0u,a,n);}
DepthWord unit__AI__SetCastleDefense(DepthRuntime&rt,const DepthWord*a,std::size_t n){return InvokeDepth(rt,0x00418EA8u,a,n);}
DepthWord unit__AI__SetCastleOffense(DepthRuntime&rt,const DepthWord*a,std::size_t n){return InvokeDepth(rt,0x00419148u,a,n);}
DepthWord castle__Golem__GetData(DepthRuntime&rt,const DepthWord*a,std::size_t n){return InvokeDepth(rt,0x00491008u,a,n);}
DepthWord castle__Golem__Data__DisposUnit(DepthRuntime&rt,const DepthWord*a,std::size_t n){return InvokeDepth(rt,0x005419C8u,a,n);}
DepthWord castle__Lilith__DisposUnit(DepthRuntime&rt,const DepthWord*a,std::size_t n){return InvokeDepth(rt,0x00541C44u,a,n);}
}
