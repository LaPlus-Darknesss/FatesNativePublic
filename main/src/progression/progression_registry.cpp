#include "fates/progression/progression_support.hpp"
#include <algorithm>

namespace fates::progression {
namespace {
constexpr ProgressionSpec kSpecs[] = {
    {"GameProfile__UpdateReliance", 0x0019B1D8u, "GameProfile__UpdateReliance", ProgressionFamily::RelianceCore},
    {"RelianceObj__RelianceObj", 0x001A3118u, "RelianceObj__RelianceObj", ProgressionFamily::RelianceCore},
    {"SupportNode__SupportNode", 0x001A5FC4u, "SupportNode__SupportNode", ProgressionFamily::SupportState},
    {"SupportPool__Deserialize", 0x001A6008u, "SupportPool__Deserialize", ProgressionFamily::SupportState},
    {"SupportPool__Entry", 0x001A61D0u, "SupportPool__Entry", ProgressionFamily::SupportState},
    {"SupportPool__GetEmpty", 0x001A62B0u, "SupportPool__GetEmpty", ProgressionFamily::SupportState},
    {"GrowSequence__BranchSkill", 0x001B3D84u, "GrowSequence__BranchSkill", ProgressionFamily::GrowthRuntime},
    {"GrowSequence__ClassChange", 0x001B3DD8u, "GrowSequence__ClassChange", ProgressionFamily::GrowthRuntime},
    {"GrowSequence__DualLevelUp", 0x001B3DF0u, "GrowSequence__DualLevelUp", ProgressionFamily::GrowthRuntime},
    {"GrowSequence__ForgetSkill", 0x001B3E1Cu, "GrowSequence__ForgetSkill", ProgressionFamily::GrowthRuntime},
    {"GrowSequence__LevelUpShow", 0x001B3F74u, "GrowSequence__LevelUpShow", ProgressionFamily::GrowthRuntime},
    {"GrowSequence__DualGainSkill", 0x001B3F94u, "GrowSequence__DualGainSkill", ProgressionFamily::GrowthRuntime},
    {"GrowSequence__GainWeaponExp", 0x001B40C8u, "GrowSequence__GainWeaponExp", ProgressionFamily::GrowthRuntime},
    {"GrowSequence__WeaponLevelUp", 0x001B40ECu, "GrowSequence__WeaponLevelUp", ProgressionFamily::GrowthRuntime},
    {"GrowSequence__ClassChangeShow", 0x001B42D0u, "GrowSequence__ClassChangeShow", ProgressionFamily::GrowthRuntime},
    {"GrowSequence__DualForgetSkill", 0x001B42F0u, "GrowSequence__DualForgetSkill", ProgressionFamily::GrowthRuntime},
    {"GrowSequence__DualLevelUpShow", 0x001B4454u, "GrowSequence__DualLevelUpShow", ProgressionFamily::GrowthRuntime},
    {"GrowSequence__DualGainWeaponExp", 0x001B4474u, "GrowSequence__DualGainWeaponExp", ProgressionFamily::GrowthRuntime},
    {"GrowSequence__ClassChangeReflect", 0x001B4610u, "GrowSequence__ClassChangeReflect", ProgressionFamily::GrowthRuntime},
    {"GrowSequence__DualLevelUpReflect", 0x001B46A8u, "GrowSequence__DualLevelUpReflect", ProgressionFamily::GrowthRuntime},
    {"GrowSequence__ClassChangeCalculate", 0x001B46E4u, "GrowSequence__ClassChangeCalculate", ProgressionFamily::GrowthRuntime},
    {"GrowSequence__DualLevelUpCalculate", 0x001B4768u, "GrowSequence__DualLevelUpCalculate", ProgressionFamily::GrowthRuntime},
    {"GrowSequence__Create", 0x001B47C0u, "GrowSequence__Create", ProgressionFamily::GrowthRuntime},
    {"GrowSequence__GainExp", 0x001B4888u, "GrowSequence__GainExp", ProgressionFamily::GrowthRuntime},
    {"GrowSequence__LevelUp", 0x001B48C4u, "GrowSequence__LevelUp", ProgressionFamily::GrowthRuntime},
    {"GrowSequence__Calculate", 0x001B48E8u, "GrowSequence__Calculate", ProgressionFamily::GrowthRuntime},
    {"GrowSequence__GainSkill", 0x001B4A08u, "GrowSequence__GainSkill", ProgressionFamily::GrowthRuntime},
    {"GrowSequence__GrowSequence", 0x001B4B9Cu, "GrowSequence__GrowSequence", ProgressionFamily::GrowthRuntime},
    {"Live2DDefine__GetSupportPoint", 0x001B59A8u, "Live2DDefine__GetSupportPoint", ProgressionFamily::RelianceCore},
    {"GrowSequence__ResumeGameInfo", 0x001BBB04u, "GrowSequence__ResumeGameInfo", ProgressionFamily::GrowthRuntime},
    {"LevelUpSequence__anonymous_namespace__ProcGrowUp__Persistent", 0x001E0D4Cu, "LevelUpSequence__anonymous_namespace__ProcGrowUp__Persistent", ProgressionFamily::LevelUpPresentation},
    {"LevelUpSequence__anonymous_namespace__ProcGrowUp__TickParams", 0x001E0E2Cu, "LevelUpSequence__anonymous_namespace__ProcGrowUp__TickParams", ProgressionFamily::LevelUpPresentation},
    {"LevelUpSequence__anonymous_namespace__ProcGrowUp__WindowOpen", 0x001E1154u, "LevelUpSequence__anonymous_namespace__ProcGrowUp__WindowOpen", ProgressionFamily::LevelUpPresentation},
    {"LevelUpSequence__anonymous_namespace__ProcGrowUp__WindowClose", 0x001E1384u, "LevelUpSequence__anonymous_namespace__ProcGrowUp__WindowClose", ProgressionFamily::LevelUpPresentation},
    {"LevelUpSequence__anonymous_namespace__ProcGrowUp__TickWaitLoad", 0x001E16D0u, "LevelUpSequence__anonymous_namespace__ProcGrowUp__TickWaitLoad", ProgressionFamily::LevelUpPresentation},
    {"LevelUpSequence__anonymous_namespace__ProcGrowUp__ResultMessage", 0x001E1770u, "LevelUpSequence__anonymous_namespace__ProcGrowUp__ResultMessage", ProgressionFamily::LevelUpPresentation},
    {"LevelUpSequence__anonymous_namespace__ProcGrowUp__CreateLightEffect", 0x001E1B98u, "LevelUpSequence__anonymous_namespace__ProcGrowUp__CreateLightEffect", ProgressionFamily::LevelUpPresentation},
    {"LevelUpSequence__anonymous_namespace__ProcGrowUp__Telop", 0x001E1C44u, "LevelUpSequence__anonymous_namespace__ProcGrowUp__Telop", ProgressionFamily::LevelUpPresentation},
    {"LevelUpSequence__anonymous_namespace__ProcGrowUp__LevelTo", 0x001E1D0Cu, "LevelUpSequence__anonymous_namespace__ProcGrowUp__LevelTo", ProgressionFamily::LevelUpPresentation},
    {"LevelUpSequence__anonymous_namespace__ProcGrowUp__LevelTo2", 0x001E1DA0u, "LevelUpSequence__anonymous_namespace__ProcGrowUp__LevelTo2", ProgressionFamily::LevelUpPresentation},
    {"LevelUpSequence__anonymous_namespace__ProcGrowUp__TickFace", 0x001E1DACu, "LevelUpSequence__anonymous_namespace__ProcGrowUp__TickFace", ProgressionFamily::LevelUpPresentation},
    {"LevelUpSequence__anonymous_namespace__ProcGrowUp__ProcGrowUp", 0x001E1EE8u, "LevelUpSequence__anonymous_namespace__ProcGrowUp__ProcGrowUp", ProgressionFamily::LevelUpPresentation},
    {"LevelUpSequence__anonymous_namespace__ProcGrowMessage__Persistent", 0x001E1F8Cu, "LevelUpSequence__anonymous_namespace__ProcGrowMessage__Persistent", ProgressionFamily::LevelUpPresentation},
    {"LevelUpSequence__anonymous_namespace__ProcGrowMessage__TickFadeOut", 0x001E1FDCu, "LevelUpSequence__anonymous_namespace__ProcGrowMessage__TickFadeOut", ProgressionFamily::LevelUpPresentation},
    {"LevelUpSequence__anonymous_namespace__ProcGrowMessage__TickScrollIn", 0x001E2048u, "LevelUpSequence__anonymous_namespace__ProcGrowMessage__TickScrollIn", ProgressionFamily::LevelUpPresentation},
    {"LevelUpSequence__anonymous_namespace__ProcGrowMessage__TickScrollUp", 0x001E2108u, "LevelUpSequence__anonymous_namespace__ProcGrowMessage__TickScrollUp", ProgressionFamily::LevelUpPresentation},
    {"LevelUpSequence__anonymous_namespace__ProcGrowMessage__TickScrollOut", 0x001E2198u, "LevelUpSequence__anonymous_namespace__ProcGrowMessage__TickScrollOut", ProgressionFamily::LevelUpPresentation},
    {"LevelUpSequence__Create", 0x001E226Cu, "LevelUpSequence__Create", ProgressionFamily::LevelUpPresentation},
    {"RelianceTalkSequence__Create", 0x0021904Cu, "RelianceTalkSequence__Create", ProgressionFamily::RelianceCore},
    {"RelianceTalkSequence__CanTalk", 0x00219218u, "RelianceTalkSequence__CanTalk", ProgressionFamily::RelianceCore},
    {"PersonEnumerator_Reliance__IsExclusion", 0x002220B8u, "PersonEnumerator_Reliance__IsExclusion", ProgressionFamily::RelianceCore},
    {"PersonEnumerator_Reliance__RelianceObjVectorList__RelianceObjVectorList", 0x00222978u, "PersonEnumerator_Reliance__RelianceObjVectorList__RelianceObjVectorList", ProgressionFamily::RelianceCore},
    {"PersonEnumerator_Reliance__PersonEnumerator_Reliance", 0x002229D0u, "PersonEnumerator_Reliance__PersonEnumerator_Reliance", ProgressionFamily::RelianceCore},
    {"map__DualSupportCalculator__Clear", 0x00370A8Cu, "map__DualSupportCalculator__Clear", ProgressionFamily::DualSupport},
    {"map__DualSupportCalculator__Calculate", 0x00370AB8u, "map__DualSupportCalculator__Calculate", ProgressionFamily::DualSupport},
    {"GrowSequence__LevelUpReflect", 0x003D4FD0u, "GrowSequence__LevelUpReflect", ProgressionFamily::GrowthRuntime},
    {"GrowSequence__LevelUpCalculate", 0x003D80FCu, "GrowSequence__LevelUpCalculate", ProgressionFamily::GrowthRuntime},
    {"SupportNode__GetName", 0x00508DB8u, "SupportNode__GetName", ProgressionFamily::SupportState},
    {"SupportPool__GetLockUnitNum", 0x00508DF8u, "SupportPool__GetLockUnitNum", ProgressionFamily::SupportState},
    {"SupportPool__Dump", 0x00508E28u, "SupportPool__Dump", ProgressionFamily::SupportState},
    {"SupportPool__Search", 0x00508E44u, "SupportPool__Search", ProgressionFamily::SupportState},
    {"SupportPool__Search_2", 0x00508ED4u, "SupportPool__Search_2", ProgressionFamily::SupportState},
    {"SupportPool__IsLocked", 0x00508F64u, "SupportPool__IsLocked", ProgressionFamily::SupportState},
    {"SupportPool__Serialize", 0x00509004u, "SupportPool__Serialize", ProgressionFamily::SupportState},
};
}
const ProgressionSpec* GetProgressionSpecs() { return kSpecs; }
std::size_t GetProgressionSpecCount() { return sizeof(kSpecs) / sizeof(kSpecs[0]); }
const ProgressionSpec* FindProgressionSpec(std::uint32_t retail_address) {
    const auto* begin = kSpecs;
    const auto* end = kSpecs + GetProgressionSpecCount();
    const auto it = std::find_if(begin, end, [retail_address](const ProgressionSpec& s) { return s.retail_address == retail_address; });
    return it == end ? nullptr : it;
}
ProgressionWord InvokeProgression(ProgressionRuntime& runtime, std::uint32_t retail_address, const ProgressionWord* args, std::size_t argc) {
    const ProgressionSpec* spec = FindProgressionSpec(retail_address);
    if (spec == nullptr || runtime.invoke == nullptr) return 0;
    return runtime.invoke(runtime.user, *spec, args, argc);
}
} // namespace fates::progression
