#include "fates/progression/progression_support_dependencies.hpp"

namespace fates::progression {

// Retail ownership is preserved here while unresolved object layouts stay behind DependencyRuntime.
DependencyWord ProcGameInfo__ChangeViewer(DependencyRuntime& runtime, const DependencyWord* args, std::size_t argc) { return InvokeDependency(runtime, 0x001BB7FCu, args, argc); }
DependencyWord ProcGameInfo__ChangeViewerUnitStatus(DependencyRuntime& runtime, const DependencyWord* args, std::size_t argc) { return InvokeDependency(runtime, 0x001BBC54u, args, argc); }
DependencyWord map__Gradation__Get(DependencyRuntime& runtime, const DependencyWord* args, std::size_t argc) { return InvokeDependency(runtime, 0x003A58E0u, args, argc); }
DependencyWord game__menu__SplitMenu__SplitMenu(DependencyRuntime& runtime, const DependencyWord* args, std::size_t argc) { return InvokeDependency(runtime, 0x003DCB38u, args, argc); }
DependencyWord game__graphics__GrowWindow__GetEffectY(DependencyRuntime& runtime, const DependencyWord* args, std::size_t argc) { return InvokeDependency(runtime, 0x003E39F8u, args, argc); }
DependencyWord game__graphics__GrowWindow__Draw(DependencyRuntime& runtime, const DependencyWord* args, std::size_t argc) { return InvokeDependency(runtime, 0x003E4220u, args, argc); }
DependencyWord game__graphics__GrowWindow__GrowWindow(DependencyRuntime& runtime, const DependencyWord* args, std::size_t argc) { return InvokeDependency(runtime, 0x003E44F8u, args, argc); }
DependencyWord game__graphics__GrowMessage__GrowMessage(DependencyRuntime& runtime, const DependencyWord* args, std::size_t argc) { return InvokeDependency(runtime, 0x003E46D8u, args, argc); }
DependencyWord game__graphics__GrowLevelUpPlate__GrowLevelUpPlate(DependencyRuntime& runtime, const DependencyWord* args, std::size_t argc) { return InvokeDependency(runtime, 0x003E7B54u, args, argc); }
DependencyWord game__graphics__GrowClassChangePlate__SetUnitIconFrom(DependencyRuntime& runtime, const DependencyWord* args, std::size_t argc) { return InvokeDependency(runtime, 0x003E8AA0u, args, argc); }
DependencyWord game__graphics__GrowClassChangePlate__GrowClassChangePlate(DependencyRuntime& runtime, const DependencyWord* args, std::size_t argc) { return InvokeDependency(runtime, 0x003E8D88u, args, argc); }
DependencyWord game__graphics__GrowClassChangePlate__GrowClassChangePlate_2(DependencyRuntime& runtime, const DependencyWord* args, std::size_t argc) { return InvokeDependency(runtime, 0x003E8DE8u, args, argc); }
DependencyWord game__graphics__Wallpaper__Process__Create(DependencyRuntime& runtime, const DependencyWord* args, std::size_t argc) { return InvokeDependency(runtime, 0x003EBCACu, args, argc); }
DependencyWord util__ColorFader__SetColor(DependencyRuntime& runtime, const DependencyWord* args, std::size_t argc) { return InvokeDependency(runtime, 0x0041BE20u, args, argc); }
DependencyWord util__Carrier__SetPosition(DependencyRuntime& runtime, const DependencyWord* args, std::size_t argc) { return InvokeDependency(runtime, 0x0041C2C8u, args, argc); }
DependencyWord util__Carrier__SetX(DependencyRuntime& runtime, const DependencyWord* args, std::size_t argc) { return InvokeDependency(runtime, 0x0041C368u, args, argc); }
DependencyWord Castle__GetData(DependencyRuntime& runtime, const DependencyWord* args, std::size_t argc) { return InvokeDependency(runtime, 0x0044545Cu, args, argc); }
DependencyWord GameFont__AddIcon(DependencyRuntime& runtime, const DependencyWord* args, std::size_t argc) { return InvokeDependency(runtime, 0x004E72CCu, args, argc); }
DependencyWord GameFont__AddSkill(DependencyRuntime& runtime, const DependencyWord* args, std::size_t argc) { return InvokeDependency(runtime, 0x004E739Cu, args, argc); }
DependencyWord ProcInst__WaitMSec(DependencyRuntime& runtime, const DependencyWord* args, std::size_t argc) { return InvokeDependency(runtime, 0x004EE2DCu, args, argc); }
DependencyWord game__graphics__GrowClassChangePlate__SetUnitIconTo(DependencyRuntime& runtime, const DependencyWord* args, std::size_t argc) { return InvokeDependency(runtime, 0x004F525Cu, args, argc); }
DependencyWord game__graphics__GrowMessage__Draw(DependencyRuntime& runtime, const DependencyWord* args, std::size_t argc) { return InvokeDependency(runtime, 0x005375ECu, args, argc); }

} // namespace fates::progression
