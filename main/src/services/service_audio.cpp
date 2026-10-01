#include "fates/services/core_services.hpp"

namespace fates::services {

// Audio ownership: retail identity is exact; unresolved object layout stays behind ServiceRuntime.

ServiceWord Sound3D__Tick(ServiceRuntime& runtime, const ServiceWord* args, std::size_t argc) {
    return InvokeService(runtime, 0x00109480u, args, argc);
}

ServiceWord Sound3D__Finalize(ServiceRuntime& runtime, const ServiceWord* args, std::size_t argc) {
    return InvokeService(runtime, 0x00109664u, args, argc);
}

ServiceWord Sound__Destruct(ServiceRuntime& runtime, const ServiceWord* args, std::size_t argc) {
    return InvokeService(runtime, 0x001BF49Cu, args, argc);
}

ServiceWord Sound__Initialize(ServiceRuntime& runtime, const ServiceWord* args, std::size_t argc) {
    return InvokeService(runtime, 0x0041F1DCu, args, argc);
}

ServiceWord Sound__SetVolumeSE(ServiceRuntime& runtime, const ServiceWord* args, std::size_t argc) {
    return InvokeService(runtime, 0x0041F49Cu, args, argc);
}

ServiceWord Sound__SEIsPrepared(ServiceRuntime& runtime, const ServiceWord* args, std::size_t argc) {
    return InvokeService(runtime, 0x0041F5ACu, args, argc);
}

ServiceWord Sound__SetVolumeBGM(ServiceRuntime& runtime, const ServiceWord* args, std::size_t argc) {
    return InvokeService(runtime, 0x0041F5F4u, args, argc);
}

ServiceWord Sound__StopAllVoice(ServiceRuntime& runtime, const ServiceWord* args, std::size_t argc) {
    return InvokeService(runtime, 0x0041F6C4u, args, argc);
}

ServiceWord Sound__DLCSoundUpdate(ServiceRuntime& runtime, const ServiceWord* args, std::size_t argc) {
    return InvokeService(runtime, 0x0041F774u, args, argc);
}

ServiceWord Sound__SEPlayPrepared(ServiceRuntime& runtime, const ServiceWord* args, std::size_t argc) {
    return InvokeService(runtime, 0x0041F834u, args, argc);
}

ServiceWord Sound__SetVolumeSysSE(ServiceRuntime& runtime, const ServiceWord* args, std::size_t argc) {
    return InvokeService(runtime, 0x0041F880u, args, argc);
}

ServiceWord Sound__SetVolumeVoice(ServiceRuntime& runtime, const ServiceWord* args, std::size_t argc) {
    return InvokeService(runtime, 0x0041F938u, args, argc);
}

ServiceWord Sound__BGMPlayPosition(ServiceRuntime& runtime, const ServiceWord* args, std::size_t argc) {
    return InvokeService(runtime, 0x0041FA08u, args, argc);
}

ServiceWord Sound__SetVoiceCallback(ServiceRuntime& runtime, const ServiceWord* args, std::size_t argc) {
    return InvokeService(runtime, 0x0041FAC4u, args, argc);
}

ServiceWord Sound__CallVoiceCallback(ServiceRuntime& runtime, const ServiceWord* args, std::size_t argc) {
    return InvokeService(runtime, 0x0041FAD4u, args, argc);
}

ServiceWord Sound__DisableBGMCommand(ServiceRuntime& runtime, const ServiceWord* args, std::size_t argc) {
    return InvokeService(runtime, 0x0041FB14u, args, argc);
}

ServiceWord Sound__LoadGroupCommonAsync(ServiceRuntime& runtime, const ServiceWord* args, std::size_t argc) {
    return InvokeService(runtime, 0x0041FB70u, args, argc);
}

ServiceWord Sound__OnPlugHeadphoneChange(ServiceRuntime& runtime, const ServiceWord* args, std::size_t argc) {
    return InvokeService(runtime, 0x0041FB98u, args, argc);
}

ServiceWord Sound__DLC__IsDLCSound(ServiceRuntime& runtime, const ServiceWord* args, std::size_t argc) {
    return InvokeService(runtime, 0x0041FD08u, args, argc);
}

ServiceWord Sound__DLC__GetDLCIndex(ServiceRuntime& runtime, const ServiceWord* args, std::size_t argc) {
    return InvokeService(runtime, 0x0041FD28u, args, argc);
}

ServiceWord Sound__DLC__VoiceArchiveFree(ServiceRuntime& runtime, const ServiceWord* args, std::size_t argc) {
    return InvokeService(runtime, 0x0041FD70u, args, argc);
}

ServiceWord Sound__DLC__VoiceArchiveLoad(ServiceRuntime& runtime, const ServiceWord* args, std::size_t argc) {
    return InvokeService(runtime, 0x0041FE34u, args, argc);
}

ServiceWord Sound__DLC__StreamArchiveFree(ServiceRuntime& runtime, const ServiceWord* args, std::size_t argc) {
    return InvokeService(runtime, 0x0041FF38u, args, argc);
}

ServiceWord Sound__DLC__StreamArchiveLoad(ServiceRuntime& runtime, const ServiceWord* args, std::size_t argc) {
    return InvokeService(runtime, 0x0041FF50u, args, argc);
}

ServiceWord Sound__DLC__GetDelay(ServiceRuntime& runtime, const ServiceWord* args, std::size_t argc) {
    return InvokeService(runtime, 0x0041FF6Cu, args, argc);
}

ServiceWord Sound__DLC__IsDLCUnit(ServiceRuntime& runtime, const ServiceWord* args, std::size_t argc) {
    return InvokeService(runtime, 0x0041FFC4u, args, argc);
}

ServiceWord Sound__MEPlay(ServiceRuntime& runtime, const ServiceWord* args, std::size_t argc) {
    return InvokeService(runtime, 0x00420080u, args, argc);
}

ServiceWord Sound__SEStop(ServiceRuntime& runtime, const ServiceWord* args, std::size_t argc) {
    return InvokeService(runtime, 0x00420124u, args, argc);
}

ServiceWord Sound__LSEStop(ServiceRuntime& runtime, const ServiceWord* args, std::size_t argc) {
    return InvokeService(runtime, 0x00420350u, args, argc);
}

ServiceWord Sound__LSEStop_2(ServiceRuntime& runtime, const ServiceWord* args, std::size_t argc) {
    return InvokeService(runtime, 0x0042037Cu, args, argc);
}

ServiceWord Sound__StopAll(ServiceRuntime& runtime, const ServiceWord* args, std::size_t argc) {
    return InvokeService(runtime, 0x004203A0u, args, argc);
}

ServiceWord Sound__BGMPause(ServiceRuntime& runtime, const ServiceWord* args, std::size_t argc) {
    return InvokeService(runtime, 0x004203C0u, args, argc);
}

ServiceWord Sound__SEPlay2D(ServiceRuntime& runtime, const ServiceWord* args, std::size_t argc) {
    return InvokeService(runtime, 0x00420548u, args, argc);
}

ServiceWord Sound__SEPlay2D_2(ServiceRuntime& runtime, const ServiceWord* args, std::size_t argc) {
    return InvokeService(runtime, 0x00420554u, args, argc);
}

ServiceWord Sound__SEPlay3D(ServiceRuntime& runtime, const ServiceWord* args, std::size_t argc) {
    return InvokeService(runtime, 0x00420744u, args, argc);
}

ServiceWord Sound__BGMResume(ServiceRuntime& runtime, const ServiceWord* args, std::size_t argc) {
    return InvokeService(runtime, 0x00420808u, args, argc);
}

ServiceWord Sound__Construct(ServiceRuntime& runtime, const ServiceWord* args, std::size_t argc) {
    return InvokeService(runtime, 0x004208A8u, args, argc);
}

ServiceWord Sound__EnvVolume(ServiceRuntime& runtime, const ServiceWord* args, std::size_t argc) {
    return InvokeService(runtime, 0x004209B4u, args, argc);
}

ServiceWord Sound__GetCutOff(ServiceRuntime& runtime, const ServiceWord* args, std::size_t argc) {
    return InvokeService(runtime, 0x004209E4u, args, argc);
}

ServiceWord Sound__IsVoicing(ServiceRuntime& runtime, const ServiceWord* args, std::size_t argc) {
    return InvokeService(runtime, 0x004209F0u, args, argc);
}

ServiceWord Sound__LSEVolume(ServiceRuntime& runtime, const ServiceWord* args, std::size_t argc) {
    return InvokeService(runtime, 0x00420A58u, args, argc);
}

ServiceWord Sound__SEPrepare(ServiceRuntime& runtime, const ServiceWord* args, std::size_t argc) {
    return InvokeService(runtime, 0x00420A9Cu, args, argc);
}

ServiceWord Sound__StopAllSE(ServiceRuntime& runtime, const ServiceWord* args, std::size_t argc) {
    return InvokeService(runtime, 0x00420AD0u, args, argc);
}

ServiceWord Sound3D__Initialize(ServiceRuntime& runtime, const ServiceWord* args, std::size_t argc) {
    return InvokeService(runtime, 0x004DFEE0u, args, argc);
}

ServiceWord Sound3D__GetRentCount(ServiceRuntime& runtime, const ServiceWord* args, std::size_t argc) {
    return InvokeService(runtime, 0x004DFF18u, args, argc);
}

ServiceWord Sound3D__SetListenerMtx(ServiceRuntime& runtime, const ServiceWord* args, std::size_t argc) {
    return InvokeService(runtime, 0x004DFF54u, args, argc);
}

ServiceWord Sound3D__RentSoundActor3D(ServiceRuntime& runtime, const ServiceWord* args, std::size_t argc) {
    return InvokeService(runtime, 0x004DFF84u, args, argc);
}

ServiceWord Sound3D__ReturnSoundActor3D(ServiceRuntime& runtime, const ServiceWord* args, std::size_t argc) {
    return InvokeService(runtime, 0x004DFFE8u, args, argc);
}

ServiceWord Sound3D__FadeoutMasterVolume(ServiceRuntime& runtime, const ServiceWord* args, std::size_t argc) {
    return InvokeService(runtime, 0x004DFFF8u, args, argc);
}

ServiceWord Sound3D__CalcDefaultListenerMatrix(ServiceRuntime& runtime, const ServiceWord* args, std::size_t argc) {
    return InvokeService(runtime, 0x004E00B0u, args, argc);
}

ServiceWord Sound3D__Sound3D(ServiceRuntime& runtime, const ServiceWord* args, std::size_t argc) {
    return InvokeService(runtime, 0x004E028Cu, args, argc);
}

} // namespace fates::services
