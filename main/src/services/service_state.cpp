#include "fates/services/core_services.hpp"

namespace fates::services {

// State ownership: retail identity is exact; unresolved object layout stays behind ServiceRuntime.

ServiceWord FlagManager__Reset(ServiceRuntime& runtime, const ServiceWord* args, std::size_t argc) {
    return InvokeService(runtime, 0x0011F414u, args, argc);
}

ServiceWord FlagManager__ResetLocal(ServiceRuntime& runtime, const ServiceWord* args, std::size_t argc) {
    return InvokeService(runtime, 0x00199C20u, args, argc);
}

ServiceWord FlagManager__ClrNotExist(ServiceRuntime& runtime, const ServiceWord* args, std::size_t argc) {
    return InvokeService(runtime, 0x00199C80u, args, argc);
}

ServiceWord FlagManager__FlagManager(ServiceRuntime& runtime, const ServiceWord* args, std::size_t argc) {
    return InvokeService(runtime, 0x00199DCCu, args, argc);
}

ServiceWord FlagManager__FlagManager_2(ServiceRuntime& runtime, const ServiceWord* args, std::size_t argc) {
    return InvokeService(runtime, 0x00199E3Cu, args, argc);
}

ServiceWord GameUserData__Initialize(ServiceRuntime& runtime, const ServiceWord* args, std::size_t argc) {
    return InvokeService(runtime, 0x001B3764u, args, argc);
}

ServiceWord GameUserData__Deserialize(ServiceRuntime& runtime, const ServiceWord* args, std::size_t argc) {
    return InvokeService(runtime, 0x001B3788u, args, argc);
}

ServiceWord GameUserData__ReleaseBackup(ServiceRuntime& runtime, const ServiceWord* args, std::size_t argc) {
    return InvokeService(runtime, 0x001B3AB0u, args, argc);
}

ServiceWord GameUserData__SetSequenceForDesc(ServiceRuntime& runtime, const ServiceWord* args, std::size_t argc) {
    return InvokeService(runtime, 0x001B3ADCu, args, argc);
}

ServiceWord GameUserData__ClearDownloadContents(ServiceRuntime& runtime, const ServiceWord* args, std::size_t argc) {
    return InvokeService(runtime, 0x001B3AF0u, args, argc);
}

ServiceWord GameUserData__Reset(ServiceRuntime& runtime, const ServiceWord* args, std::size_t argc) {
    return InvokeService(runtime, 0x001B3B0Cu, args, argc);
}

ServiceWord GameUserData__SetMode(ServiceRuntime& runtime, const ServiceWord* args, std::size_t argc) {
    return InvokeService(runtime, 0x001B3BB8u, args, argc);
}

ServiceWord GameUserData__Finalize(ServiceRuntime& runtime, const ServiceWord* args, std::size_t argc) {
    return InvokeService(runtime, 0x001B3BE8u, args, argc);
}

ServiceWord GameUserData__GameUserData(ServiceRuntime& runtime, const ServiceWord* args, std::size_t argc) {
    return InvokeService(runtime, 0x001B3C68u, args, argc);
}

ServiceWord ContentsLocal__Deserialize(ServiceRuntime& runtime, const ServiceWord* args, std::size_t argc) {
    return InvokeService(runtime, 0x001C71E8u, args, argc);
}

ServiceWord ContentsLocal__Reset(ServiceRuntime& runtime, const ServiceWord* args, std::size_t argc) {
    return InvokeService(runtime, 0x001C7218u, args, argc);
}

ServiceWord DeliveryMessage__Commit(ServiceRuntime& runtime, const ServiceWord* args, std::size_t argc) {
    return InvokeService(runtime, 0x001DE1ECu, args, argc);
}

ServiceWord VariableManager__ResetLocal(ServiceRuntime& runtime, const ServiceWord* args, std::size_t argc) {
    return InvokeService(runtime, 0x001E7DD8u, args, argc);
}

ServiceWord VariableManager__ClrNotExist(ServiceRuntime& runtime, const ServiceWord* args, std::size_t argc) {
    return InvokeService(runtime, 0x001E7E1Cu, args, argc);
}

ServiceWord VariableManager__Deserialize(ServiceRuntime& runtime, const ServiceWord* args, std::size_t argc) {
    return InvokeService(runtime, 0x001E7E5Cu, args, argc);
}

ServiceWord VariableManager__Reset(ServiceRuntime& runtime, const ServiceWord* args, std::size_t argc) {
    return InvokeService(runtime, 0x001E8080u, args, argc);
}

ServiceWord VariableManager__Serialize(ServiceRuntime& runtime, const ServiceWord* args, std::size_t argc) {
    return InvokeService(runtime, 0x001E80A8u, args, argc);
}

ServiceWord VariableManager__VariableManager(ServiceRuntime& runtime, const ServiceWord* args, std::size_t argc) {
    return InvokeService(runtime, 0x001E80E0u, args, argc);
}

ServiceWord VariableManager__VariableManager_2(ServiceRuntime& runtime, const ServiceWord* args, std::size_t argc) {
    return InvokeService(runtime, 0x001E8148u, args, argc);
}

ServiceWord GameSkipSequenceHelper__WaitSkip(ServiceRuntime& runtime, const ServiceWord* args, std::size_t argc) {
    return InvokeService(runtime, 0x0021DD28u, args, argc);
}

ServiceWord FlagManager__Deserialize(ServiceRuntime& runtime, const ServiceWord* args, std::size_t argc) {
    return InvokeService(runtime, 0x002FDEE8u, args, argc);
}

ServiceWord GameUserData__IsRouteOrigin(ServiceRuntime& runtime, const ServiceWord* args, std::size_t argc) {
    return InvokeService(runtime, 0x00449C38u, args, argc);
}

ServiceWord FlagManager__Serialize(ServiceRuntime& runtime, const ServiceWord* args, std::size_t argc) {
    return InvokeService(runtime, 0x0044B994u, args, argc);
}

ServiceWord DeliveryMessage__Deserialize(ServiceRuntime& runtime, const ServiceWord* args, std::size_t argc) {
    return InvokeService(runtime, 0x0044C0DCu, args, argc);
}

ServiceWord GameSkip__Tick(ServiceRuntime& runtime, const ServiceWord* args, std::size_t argc) {
    return InvokeService(runtime, 0x004EB4ECu, args, argc);
}

ServiceWord GameSkip__Escape(ServiceRuntime& runtime, const ServiceWord* args, std::size_t argc) {
    return InvokeService(runtime, 0x004EB814u, args, argc);
}

ServiceWord GameSkip__IsWait(ServiceRuntime& runtime, const ServiceWord* args, std::size_t argc) {
    return InvokeService(runtime, 0x004EB840u, args, argc);
}

ServiceWord GameSkip__Render(ServiceRuntime& runtime, const ServiceWord* args, std::size_t argc) {
    return InvokeService(runtime, 0x004EB870u, args, argc);
}

ServiceWord GameSkip__GameSkip(ServiceRuntime& runtime, const ServiceWord* args, std::size_t argc) {
    return InvokeService(runtime, 0x004EBADCu, args, argc);
}

ServiceWord GameSkip__GameSkip_2(ServiceRuntime& runtime, const ServiceWord* args, std::size_t argc) {
    return InvokeService(runtime, 0x004EBAF8u, args, argc);
}

ServiceWord FlagManager__Get(ServiceRuntime& runtime, const ServiceWord* args, std::size_t argc) {
    return InvokeService(runtime, 0x00508508u, args, argc);
}

ServiceWord FlagManager__Get_2(ServiceRuntime& runtime, const ServiceWord* args, std::size_t argc) {
    return InvokeService(runtime, 0x00508584u, args, argc);
}

ServiceWord GameUserData__IsCastleTime(ServiceRuntime& runtime, const ServiceWord* args, std::size_t argc) {
    return InvokeService(runtime, 0x005094D0u, args, argc);
}

ServiceWord GameUserData__GetChapterName(ServiceRuntime& runtime, const ServiceWord* args, std::size_t argc) {
    return InvokeService(runtime, 0x0050951Cu, args, argc);
}

ServiceWord GameUserData__IsChapterRecord(ServiceRuntime& runtime, const ServiceWord* args, std::size_t argc) {
    return InvokeService(runtime, 0x00509590u, args, argc);
}

ServiceWord GameUserData__GetChapterPrefix(ServiceRuntime& runtime, const ServiceWord* args, std::size_t argc) {
    return InvokeService(runtime, 0x005095D8u, args, argc);
}

ServiceWord GameUserData__GetWinRuleMessage(ServiceRuntime& runtime, const ServiceWord* args, std::size_t argc) {
    return InvokeService(runtime, 0x0050964Cu, args, argc);
}

ServiceWord GameUserData__GetMode(ServiceRuntime& runtime, const ServiceWord* args, std::size_t argc) {
    return InvokeService(runtime, 0x005096D0u, args, argc);
}

ServiceWord GameUserData__Serialize(ServiceRuntime& runtime, const ServiceWord* args, std::size_t argc) {
    return InvokeService(runtime, 0x005096F0u, args, argc);
}

ServiceWord ContentsLocal__Serialize(ServiceRuntime& runtime, const ServiceWord* args, std::size_t argc) {
    return InvokeService(runtime, 0x0050A434u, args, argc);
}

ServiceWord DeliveryMessage__GetMessage(ServiceRuntime& runtime, const ServiceWord* args, std::size_t argc) {
    return InvokeService(runtime, 0x0050B9ACu, args, argc);
}

ServiceWord DeliveryMessage__IsExist(ServiceRuntime& runtime, const ServiceWord* args, std::size_t argc) {
    return InvokeService(runtime, 0x0050B9CCu, args, argc);
}

ServiceWord DeliveryMessage__IsChanged(ServiceRuntime& runtime, const ServiceWord* args, std::size_t argc) {
    return InvokeService(runtime, 0x0050B9DCu, args, argc);
}

ServiceWord DeliveryMessage__Serialize(ServiceRuntime& runtime, const ServiceWord* args, std::size_t argc) {
    return InvokeService(runtime, 0x0050BA10u, args, argc);
}

ServiceWord FlagNameManager__GetIndex(ServiceRuntime& runtime, const ServiceWord* args, std::size_t argc) {
    return InvokeService(runtime, 0x0050BA3Cu, args, argc);
}

} // namespace fates::services
