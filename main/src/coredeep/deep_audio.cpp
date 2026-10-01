#include "fates/coredeep/deeper_core_impl.hpp"

namespace fates::coredeep {

// AudioDeep: exact retail identity owned; unresolved concrete layout/backend state remains behind DeepRuntime.

DeepWord sound__Effector__Effector_2(DeepRuntime& runtime, const DeepWord* args, std::size_t argc) {
    return InvokeDeep(runtime, 0x004327C0u, args, argc);
}

DeepWord ProcSoundMonitor__GetInstance(DeepRuntime& runtime, const DeepWord* args, std::size_t argc) {
    return InvokeDeep(runtime, 0x001EF078u, args, argc);
}

DeepWord map__sound__anonymous_namespace__TSound__SetName(DeepRuntime& runtime, const DeepWord* args, std::size_t argc) {
    return InvokeDeep(runtime, 0x00398BCCu, args, argc);
}

DeepWord SoundHandle__Play(DeepRuntime& runtime, const DeepWord* args, std::size_t argc) {
    return InvokeDeep(runtime, 0x001A5644u, args, argc);
}

DeepWord ProcSoundMonitor__ResetPosition(DeepRuntime& runtime, const DeepWord* args, std::size_t argc) {
    return InvokeDeep(runtime, 0x001EF094u, args, argc);
}

DeepWord SoundDuplicationChecker__Initialize(DeepRuntime& runtime, const DeepWord* args, std::size_t argc) {
    return InvokeDeep(runtime, 0x00220D50u, args, argc);
}

DeepWord RandomSound__GetSoundItem(DeepRuntime& runtime, const DeepWord* args, std::size_t argc) {
    return InvokeDeep(runtime, 0x00508AC8u, args, argc);
}

} // namespace fates::coredeep
