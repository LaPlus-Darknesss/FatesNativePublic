#include "fates/progression/progression_support_depth.hpp"

namespace fates::progression {

// Retail ownership is preserved while unresolved object layouts stay behind DepthRuntime.
DepthWord ViewerSituation__InitHelp(DepthRuntime& runtime,const DepthWord* args,std::size_t argc) { return InvokeDepth(runtime,0x001E916Cu,args,argc); }
DepthWord ViewerUnitStatus__InitHelp(DepthRuntime& runtime,const DepthWord* args,std::size_t argc) { return InvokeDepth(runtime,0x001F651Cu,args,argc); }
DepthWord game__graphics__ExpWindow__SetUnit(DepthRuntime& runtime,const DepthWord* args,std::size_t argc) { return InvokeDepth(runtime,0x003EB63Cu,args,argc); }
DepthWord game__graphics__ExpWindow__ExpWindow(DepthRuntime& runtime,const DepthWord* args,std::size_t argc) { return InvokeDepth(runtime,0x003EB6C8u,args,argc); }

} // namespace fates::progression
