#include "fates/chapter/chapter_run_state_depth.hpp"
namespace fates::chapter{
// Exact retail identities are source-owned; unresolved object layouts remain behind DepthRuntime.
DepthWord MainSequence__Jump(DepthRuntime&rt,const DepthWord*a,std::size_t n){return InvokeDepth(rt,0x001B741Cu,a,n);}
DepthWord MainSequence__GetInstance(DepthRuntime&rt,const DepthWord*a,std::size_t n){return InvokeDepth(rt,0x001B5FC0u,a,n);}
DepthWord ProcGameInfo__SetMapActive(DepthRuntime&rt,const DepthWord*a,std::size_t n){return InvokeDepth(rt,0x001BB9C4u,a,n);}
DepthWord GameBackup__IsNormalizeAddContentOccurred(DepthRuntime&rt,const DepthWord*a,std::size_t n){return InvokeDepth(rt,0x00179000u,a,n);}
DepthWord GameBackup__Process__Delete(DepthRuntime&rt,const DepthWord*a,std::size_t n){return InvokeDepth(rt,0x001798BCu,a,n);}
DepthWord ProcGameInfo__EnableChangeViewer(DepthRuntime&rt,const DepthWord*a,std::size_t n){return InvokeDepth(rt,0x001BBB1Cu,a,n);}
DepthWord GameBackupHeader__Reset(DepthRuntime&rt,const DepthWord*a,std::size_t n){return InvokeDepth(rt,0x001EB818u,a,n);}
DepthWord GameBackup__Instant__ReadHeader(DepthRuntime&rt,const DepthWord*a,std::size_t n){return InvokeDepth(rt,0x001793F0u,a,n);}
DepthWord ProcGameInfo__Create(DepthRuntime&rt,const DepthWord*a,std::size_t n){return InvokeDepth(rt,0x001BBD90u,a,n);}
DepthWord ProcGameInfo__ChangeViewerNone(DepthRuntime&rt,const DepthWord*a,std::size_t n){return InvokeDepth(rt,0x001BBB68u,a,n);}
}
