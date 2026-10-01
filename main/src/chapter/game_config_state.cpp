#include "fates/chapter/chapter_run_state.hpp"
namespace fates::chapter {
// Exact retail identity is source-owned; unresolved object layouts stay behind RunStateRuntime.
RunStateWord GameConfigData__Deserialize(RunStateRuntime& rt,const RunStateWord* a,std::size_t n){return InvokeRunState(rt,0x001D0374u,a,n);}
RunStateWord GameConfigData__SetDistanceNext(RunStateRuntime& rt,const RunStateWord* a,std::size_t n){return InvokeRunState(rt,0x001D0494u,a,n);}
RunStateWord GameConfigData__DistanceNormalize(RunStateRuntime& rt,const RunStateWord* a,std::size_t n){return InvokeRunState(rt,0x001D0548u,a,n);}
RunStateWord GameConfigData__SetInfoCapability(RunStateRuntime& rt,const RunStateWord* a,std::size_t n){return InvokeRunState(rt,0x001D05E4u,a,n);}
RunStateWord GameConfigData__StartFromBeginning(RunStateRuntime& rt,const RunStateWord* a,std::size_t n){return InvokeRunState(rt,0x001D0618u,a,n);}
RunStateWord GameConfigData__Reset(RunStateRuntime& rt,const RunStateWord* a,std::size_t n){return InvokeRunState(rt,0x001D06DCu,a,n);}
RunStateWord GameConfigData__GameConfigData(RunStateRuntime& rt,const RunStateWord* a,std::size_t n){return InvokeRunState(rt,0x001D0778u,a,n);}
RunStateWord GameConfigData__SetVolumeSE(RunStateRuntime& rt,const RunStateWord* a,std::size_t n){return InvokeRunState(rt,0x0041F490u,a,n);}
RunStateWord GameConfigData__SetVolumeBGM(RunStateRuntime& rt,const RunStateWord* a,std::size_t n){return InvokeRunState(rt,0x0041F5E8u,a,n);}
RunStateWord GameConfigData__SetVolumeSysSE(RunStateRuntime& rt,const RunStateWord* a,std::size_t n){return InvokeRunState(rt,0x0041F874u,a,n);}
RunStateWord GameConfigData__SetVolumeVoice(RunStateRuntime& rt,const RunStateWord* a,std::size_t n){return InvokeRunState(rt,0x0041F92Cu,a,n);}
RunStateWord GameConfigData__IsSkipInput(RunStateRuntime& rt,const RunStateWord* a,std::size_t n){return InvokeRunState(rt,0x0050AA4Cu,a,n);}
RunStateWord GameConfigData__IsDetailBattle(RunStateRuntime& rt,const RunStateWord* a,std::size_t n){return InvokeRunState(rt,0x0050AAA0u,a,n);}
RunStateWord GameConfigData__IsDetailSupport(RunStateRuntime& rt,const RunStateWord* a,std::size_t n){return InvokeRunState(rt,0x0050AC04u,a,n);}
RunStateWord GameConfigData__GetInfoCapability(RunStateRuntime& rt,const RunStateWord* a,std::size_t n){return InvokeRunState(rt,0x0050AE40u,a,n);}
}
