#include "fates/progression/progression_support_depth.hpp"
#include <algorithm>

namespace fates::progression {
namespace {
constexpr DepthSpec kSpecs[] = {
    {"TalkWindow__FadeInFace", 0x0018E564u, "TalkWindow__FadeInFace", DepthFamily::TalkFace},
    {"TalkWindow__FadeInFaceInSkip", 0x0018F338u, "TalkWindow__FadeInFaceInSkip", DepthFamily::TalkFace},
    {"TalkWindow__Reset", 0x0018FAD0u, "TalkWindow__Reset", DepthFamily::TalkFace},
    {"FaceInstance__SetExpressionCommon", 0x001B0454u, "FaceInstance__SetExpressionCommon", DepthFamily::TalkFace},
    {"FaceInstance__ChangeEquipAccessory1", 0x001B063Cu, "FaceInstance__ChangeEquipAccessory1", DepthFamily::TalkFace},
    {"FaceInstance__ChangeEquipAccessory2", 0x001B0704u, "FaceInstance__ChangeEquipAccessory2", DepthFamily::TalkFace},
    {"FaceInstance__LoadByFid", 0x001B18E8u, "FaceInstance__LoadByFid", DepthFamily::TalkFace},
    {"FrameManager__LayoutItemIconText__LayoutItemIconText", 0x001B21B4u, "FrameManager__LayoutItemIconText__LayoutItemIconText", DepthFamily::Presentation},
    {"FrameManager__Layout__DeleteTitleItems", 0x001B2304u, "FrameManager__Layout__DeleteTitleItems", DepthFamily::Presentation},
    {"TalkExpander__ExpandMessage_2", 0x001C009Cu, "TalkExpander__ExpandMessage_2", DepthFamily::TalkFace},
    {"FaceDataManager__CreateFIDFromUnitEdit", 0x001E00D4u, "FaceDataManager__CreateFIDFromUnitEdit", DepthFamily::TalkFace},
    {"ViewerSituation__InitHelp", 0x001E916Cu, "ViewerSituation__InitHelp", DepthFamily::ExperienceViewer},
    {"ViewerUnitStatus__InitHelp", 0x001F651Cu, "ViewerUnitStatus__InitHelp", DepthFamily::ExperienceViewer},
    {"ClassChangeEnumerator__EnumerateBuddy", 0x0021A26Cu, "ClassChangeEnumerator__EnumerateBuddy", DepthFamily::ClassChangeEnumeration},
    {"ClassChangeEnumerator__EnumerateMarrige", 0x0021A614u, "ClassChangeEnumerator__EnumerateMarrige", DepthFamily::ClassChangeEnumeration},
    {"ClassChangeEnumerator__EnumerateParallel", 0x0021A8E8u, "ClassChangeEnumerator__EnumerateParallel", DepthFamily::ClassChangeEnumeration},
    {"FaceAccessoryDataManager__GetAccessory1Data_2", 0x0022133Cu, "FaceAccessoryDataManager__GetAccessory1Data_2", DepthFamily::TalkFace},
    {"FaceAccessoryDataManager__GetAccessory2Data", 0x0022135Cu, "FaceAccessoryDataManager__GetAccessory2Data", DepthFamily::TalkFace},
    {"FaceAccessoryDataManager__GetAccessoryDataByACID", 0x0022137Cu, "FaceAccessoryDataManager__GetAccessoryDataByACID", DepthFamily::TalkFace},
    {"game__graphics__GrowWindow__DrawAbility", 0x003E3A5Cu, "game__graphics__GrowWindow__DrawAbility", DepthFamily::Presentation},
    {"game__graphics__NameWindow__GetWidth", 0x003E462Cu, "game__graphics__NameWindow__GetWidth", DepthFamily::Presentation},
    {"game__graphics__AbilityWindowBase__AbilityWindowBase", 0x003E7FC4u, "game__graphics__AbilityWindowBase__AbilityWindowBase", DepthFamily::Presentation},
    {"game__graphics__ExpWindow__SetUnit", 0x003EB63Cu, "game__graphics__ExpWindow__SetUnit", DepthFamily::ExperienceViewer},
    {"game__graphics__ExpWindow__ExpWindow", 0x003EB6C8u, "game__graphics__ExpWindow__ExpWindow", DepthFamily::ExperienceViewer},
    {"unit__Identifier__Clear", 0x00418C34u, "unit__Identifier__Clear", DepthFamily::SupportPacket},
    {"util__ColorFader__ColorFader", 0x0041BEF8u, "util__ColorFader__ColorFader", DepthFamily::Presentation},
    {"util__Carrier__Carrier", 0x0041C4BCu, "util__Carrier__Carrier", DepthFamily::Presentation},
    {"TalkLog__InitializeEveryTalk", 0x004E06D0u, "TalkLog__InitializeEveryTalk", DepthFamily::TalkFace},
    {"GameFont__DrawCenterAlign", 0x004E6C3Cu, "GameFont__DrawCenterAlign", DepthFamily::Presentation},
    {"GameMenu__GameMenu", 0x004EB490u, "GameMenu__GameMenu", DepthFamily::Presentation},
    {"TalkUtil__GetTalkTone", 0x004F1FB4u, "TalkUtil__GetTalkTone", DepthFamily::TalkFace},
    {"TalkUtil__GetPlayerUnit", 0x004F2330u, "TalkUtil__GetPlayerUnit", DepthFamily::TalkFace},
    {"game__graphics__NameWindow__Draw", 0x00536E50u, "game__graphics__NameWindow__Draw", DepthFamily::Presentation},
    {"unit__Identifier__Serialize", 0x0053AA40u, "unit__Identifier__Serialize", DepthFamily::SupportPacket},
    {"unit__Edit__Serialize", 0x0053AC88u, "unit__Edit__Serialize", DepthFamily::SupportPacket},
};
}
const DepthSpec* GetDepthSpecs() { return kSpecs; }
std::size_t GetDepthSpecCount() { return sizeof(kSpecs)/sizeof(kSpecs[0]); }
const DepthSpec* FindDepthSpec(std::uint32_t retail_address) {
    const auto* begin=kSpecs; const auto* end=kSpecs+GetDepthSpecCount();
    const auto it=std::find_if(begin,end,[retail_address](const DepthSpec& s){return s.retail_address==retail_address;});
    return it==end?nullptr:it;
}
DepthWord InvokeDepth(DepthRuntime& runtime,std::uint32_t retail_address,const DepthWord* args,std::size_t argc) {
    const DepthSpec* spec=FindDepthSpec(retail_address);
    if(spec==nullptr || runtime.invoke==nullptr) return 0;
    return runtime.invoke(runtime.user,*spec,args,argc);
}
} // namespace fates::progression
