#include "fates/progression/progression_support_depth.hpp"

namespace fates::progression {

// Retail ownership is preserved while unresolved object layouts stay behind DepthRuntime.
DepthWord TalkWindow__FadeInFace(DepthRuntime& runtime,const DepthWord* args,std::size_t argc) { return InvokeDepth(runtime,0x0018E564u,args,argc); }
DepthWord TalkWindow__FadeInFaceInSkip(DepthRuntime& runtime,const DepthWord* args,std::size_t argc) { return InvokeDepth(runtime,0x0018F338u,args,argc); }
DepthWord TalkWindow__Reset(DepthRuntime& runtime,const DepthWord* args,std::size_t argc) { return InvokeDepth(runtime,0x0018FAD0u,args,argc); }
DepthWord FaceInstance__SetExpressionCommon(DepthRuntime& runtime,const DepthWord* args,std::size_t argc) { return InvokeDepth(runtime,0x001B0454u,args,argc); }
DepthWord FaceInstance__ChangeEquipAccessory1(DepthRuntime& runtime,const DepthWord* args,std::size_t argc) { return InvokeDepth(runtime,0x001B063Cu,args,argc); }
DepthWord FaceInstance__ChangeEquipAccessory2(DepthRuntime& runtime,const DepthWord* args,std::size_t argc) { return InvokeDepth(runtime,0x001B0704u,args,argc); }
DepthWord FaceInstance__LoadByFid(DepthRuntime& runtime,const DepthWord* args,std::size_t argc) { return InvokeDepth(runtime,0x001B18E8u,args,argc); }
DepthWord TalkExpander__ExpandMessage_2(DepthRuntime& runtime,const DepthWord* args,std::size_t argc) { return InvokeDepth(runtime,0x001C009Cu,args,argc); }
DepthWord FaceDataManager__CreateFIDFromUnitEdit(DepthRuntime& runtime,const DepthWord* args,std::size_t argc) { return InvokeDepth(runtime,0x001E00D4u,args,argc); }
DepthWord FaceAccessoryDataManager__GetAccessory1Data_2(DepthRuntime& runtime,const DepthWord* args,std::size_t argc) { return InvokeDepth(runtime,0x0022133Cu,args,argc); }
DepthWord FaceAccessoryDataManager__GetAccessory2Data(DepthRuntime& runtime,const DepthWord* args,std::size_t argc) { return InvokeDepth(runtime,0x0022135Cu,args,argc); }
DepthWord FaceAccessoryDataManager__GetAccessoryDataByACID(DepthRuntime& runtime,const DepthWord* args,std::size_t argc) { return InvokeDepth(runtime,0x0022137Cu,args,argc); }
DepthWord TalkLog__InitializeEveryTalk(DepthRuntime& runtime,const DepthWord* args,std::size_t argc) { return InvokeDepth(runtime,0x004E06D0u,args,argc); }
DepthWord TalkUtil__GetTalkTone(DepthRuntime& runtime,const DepthWord* args,std::size_t argc) { return InvokeDepth(runtime,0x004F1FB4u,args,argc); }
DepthWord TalkUtil__GetPlayerUnit(DepthRuntime& runtime,const DepthWord* args,std::size_t argc) { return InvokeDepth(runtime,0x004F2330u,args,argc); }

} // namespace fates::progression
