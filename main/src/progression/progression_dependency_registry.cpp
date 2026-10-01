#include "fates/progression/progression_support_dependencies.hpp"
#include <algorithm>

namespace fates::progression {
namespace {
constexpr DependencySpec kSpecs[] = {
    {"ClassChange__GetType", 0x001951B0u, "ClassChange__GetType", DependencyFamily::ProgressionData},
    {"ExpSequence__Create", 0x001961F0u, "ExpSequence__Create", DependencyFamily::ProgressionData},
    {"FaceInstance__LoadByUnit", 0x001AE730u, "FaceInstance__LoadByUnit", DependencyFamily::TalkFacePresentation},
    {"FaceInstance__SetDirection", 0x001AEC00u, "FaceInstance__SetDirection", DependencyFamily::TalkFacePresentation},
    {"FaceInstance__ChangeExpression", 0x001AFA5Cu, "FaceInstance__ChangeExpression", DependencyFamily::TalkFacePresentation},
    {"FaceInstance__FaceInstance", 0x001B1A54u, "FaceInstance__FaceInstance", DependencyFamily::TalkFacePresentation},
    {"FrameManager__Layout__Show", 0x001B2924u, "FrameManager__Layout__Show", DependencyFamily::TalkFacePresentation},
    {"FrameManager__Layout__SetTitle_2", 0x001B2CC8u, "FrameManager__Layout__SetTitle_2", DependencyFamily::TalkFacePresentation},
    {"FrameManager__Layout__AddButton", 0x001B2D74u, "FrameManager__Layout__AddButton", DependencyFamily::TalkFacePresentation},
    {"ProcGameInfo__ChangeViewer", 0x001BB7FCu, "ProcGameInfo__ChangeViewer", DependencyFamily::ProgressionPresentation},
    {"ProcGameInfo__ChangeViewerUnitStatus", 0x001BBC54u, "ProcGameInfo__ChangeViewerUnitStatus", DependencyFamily::ProgressionPresentation},
    {"ProcTalkManager__InitializeDirect", 0x001E5474u, "ProcTalkManager__InitializeDirect", DependencyFamily::TalkFacePresentation},
    {"TalkCodeFactory__WindowActive", 0x001E72B0u, "TalkCodeFactory__WindowActive", DependencyFamily::TalkFacePresentation},
    {"TalkCodeFactory__WindowDelete", 0x001E7328u, "TalkCodeFactory__WindowDelete", DependencyFamily::TalkFacePresentation},
    {"TalkCodeFactory__WindowMakeGrow", 0x001E73A0u, "TalkCodeFactory__WindowMakeGrow", DependencyFamily::TalkFacePresentation},
    {"TalkCodeFactory__AddMess", 0x001E753Cu, "TalkCodeFactory__AddMess", DependencyFamily::TalkFacePresentation},
    {"TalkCodeFactory__TalkType", 0x001E75D4u, "TalkCodeFactory__TalkType", DependencyFamily::TalkFacePresentation},
    {"TalkCodeFactory__AddDirect", 0x001E7620u, "TalkCodeFactory__AddDirect", DependencyFamily::TalkFacePresentation},
    {"TalkCodeFactory__TalkCodeFactory", 0x001E7798u, "TalkCodeFactory__TalkCodeFactory", DependencyFamily::TalkFacePresentation},
    {"PersonEnumerator__PersonEnumerator_2", 0x001EE304u, "PersonEnumerator__PersonEnumerator_2", DependencyFamily::SupportState},
    {"GameUserGlobalData__Get", 0x0020C60Cu, "GameUserGlobalData__Get", DependencyFamily::SupportState},
    {"GameUserGlobalData__Viewer__GetRelianceFlagIndexPlayerAqua", 0x0020C6C8u, "GameUserGlobalData__Viewer__GetRelianceFlagIndexPlayerAqua", DependencyFamily::SupportState},
    {"ClassChangeEnumerator__GetLearnEquipSkill", 0x0021AC8Cu, "ClassChangeEnumerator__GetLearnEquipSkill", DependencyFamily::ProgressionData},
    {"VariableSizeFlagManager__Get", 0x00220F60u, "VariableSizeFlagManager__Get", DependencyFamily::SupportState},
    {"map__Gradation__Get", 0x003A58E0u, "map__Gradation__Get", DependencyFamily::ProgressionPresentation},
    {"anonymous_namespace__IsRelianceTalk", 0x003CDA04u, "anonymous_namespace__IsRelianceTalk", DependencyFamily::SupportState},
    {"game__menu__SplitMenu__SplitMenu", 0x003DCB38u, "game__menu__SplitMenu__SplitMenu", DependencyFamily::ProgressionPresentation},
    {"game__packet__Unit__Deserialize", 0x003DE0CCu, "game__packet__Unit__Deserialize", DependencyFamily::SupportPacket},
    {"game__packet__Unit__Copy", 0x003DE458u, "game__packet__Unit__Copy", DependencyFamily::SupportPacket},
    {"game__packet__Unit__Clear", 0x003DE46Cu, "game__packet__Unit__Clear", DependencyFamily::SupportPacket},
    {"game__graphics__GrowWindow__GetEffectY", 0x003E39F8u, "game__graphics__GrowWindow__GetEffectY", DependencyFamily::ProgressionPresentation},
    {"game__graphics__GrowWindow__Draw", 0x003E4220u, "game__graphics__GrowWindow__Draw", DependencyFamily::ProgressionPresentation},
    {"game__graphics__GrowWindow__GrowWindow", 0x003E44F8u, "game__graphics__GrowWindow__GrowWindow", DependencyFamily::ProgressionPresentation},
    {"game__graphics__GrowMessage__GrowMessage", 0x003E46D8u, "game__graphics__GrowMessage__GrowMessage", DependencyFamily::ProgressionPresentation},
    {"game__graphics__GrowLevelUpPlate__GrowLevelUpPlate", 0x003E7B54u, "game__graphics__GrowLevelUpPlate__GrowLevelUpPlate", DependencyFamily::ProgressionPresentation},
    {"game__graphics__GrowClassChangePlate__SetUnitIconFrom", 0x003E8AA0u, "game__graphics__GrowClassChangePlate__SetUnitIconFrom", DependencyFamily::ProgressionPresentation},
    {"game__graphics__GrowClassChangePlate__GrowClassChangePlate", 0x003E8D88u, "game__graphics__GrowClassChangePlate__GrowClassChangePlate", DependencyFamily::ProgressionPresentation},
    {"game__graphics__GrowClassChangePlate__GrowClassChangePlate_2", 0x003E8DE8u, "game__graphics__GrowClassChangePlate__GrowClassChangePlate_2", DependencyFamily::ProgressionPresentation},
    {"game__graphics__Wallpaper__Process__Create", 0x003EBCACu, "game__graphics__Wallpaper__Process__Create", DependencyFamily::ProgressionPresentation},
    {"unit__Edit__Clear", 0x00419644u, "unit__Edit__Clear", DependencyFamily::SupportPacket},
    {"unit__Cloth__operator", 0x0041ACB4u, "unit__Cloth__operator", DependencyFamily::SupportPacket},
    {"unit__Record__operator", 0x0041B1A0u, "unit__Record__operator", DependencyFamily::SupportPacket},
    {"unit__Enhance__operator", 0x0041B660u, "unit__Enhance__operator", DependencyFamily::SupportPacket},
    {"util__ColorFader__SetColor", 0x0041BE20u, "util__ColorFader__SetColor", DependencyFamily::ProgressionPresentation},
    {"util__Carrier__SetPosition", 0x0041C2C8u, "util__Carrier__SetPosition", DependencyFamily::ProgressionPresentation},
    {"util__Carrier__SetX", 0x0041C368u, "util__Carrier__SetX", DependencyFamily::ProgressionPresentation},
    {"Castle__GetData", 0x0044545Cu, "Castle__GetData", DependencyFamily::ProgressionPresentation},
    {"Random__GetValue", 0x0044ADF8u, "Random__GetValue", DependencyFamily::ProgressionData},
    {"GameFont__AddIcon", 0x004E72CCu, "GameFont__AddIcon", DependencyFamily::ProgressionPresentation},
    {"GameFont__AddSkill", 0x004E739Cu, "GameFont__AddSkill", DependencyFamily::ProgressionPresentation},
    {"ProcInst__WaitMSec", 0x004EE2DCu, "ProcInst__WaitMSec", DependencyFamily::ProgressionPresentation},
    {"game__graphics__GrowClassChangePlate__SetUnitIconTo", 0x004F525Cu, "game__graphics__GrowClassChangePlate__SetUnitIconTo", DependencyFamily::ProgressionPresentation},
    {"FlagManagerNoName__Get", 0x0050C9A4u, "FlagManagerNoName__Get", DependencyFamily::SupportState},
    {"game__packet__Unit__Serialize", 0x0053592Cu, "game__packet__Unit__Serialize", DependencyFamily::SupportPacket},
    {"game__graphics__GrowMessage__Draw", 0x005375ECu, "game__graphics__GrowMessage__Draw", DependencyFamily::ProgressionPresentation},
    {"unit__Identifier__IsEqual", 0x0053A960u, "unit__Identifier__IsEqual", DependencyFamily::SupportPacket},
};
}
const DependencySpec* GetDependencySpecs() { return kSpecs; }
std::size_t GetDependencySpecCount() { return sizeof(kSpecs) / sizeof(kSpecs[0]); }
const DependencySpec* FindDependencySpec(std::uint32_t retail_address) {
    const auto* begin = kSpecs;
    const auto* end = kSpecs + GetDependencySpecCount();
    const auto it = std::find_if(begin, end, [retail_address](const DependencySpec& s) { return s.retail_address == retail_address; });
    return it == end ? nullptr : it;
}
DependencyWord InvokeDependency(DependencyRuntime& runtime, std::uint32_t retail_address, const DependencyWord* args, std::size_t argc) {
    const DependencySpec* spec = FindDependencySpec(retail_address);
    if (spec == nullptr || runtime.invoke == nullptr) return 0;
    return runtime.invoke(runtime.user, *spec, args, argc);
}
} // namespace fates::progression
