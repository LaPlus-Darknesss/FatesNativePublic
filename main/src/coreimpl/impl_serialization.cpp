#include "fates/coreimpl/core_service_impl.hpp"

namespace fates::coreimpl {

// SerializationState: exact retail identity is owned; unresolved concrete object layout remains behind ImplRuntime.

ImplWord Stream__WriteByte(ImplRuntime& runtime, const ImplWord* args, std::size_t argc) {
    return InvokeImpl(runtime, 0x0044C3DCu, args, argc);
}

ImplWord Stream__ReadByte(ImplRuntime& runtime, const ImplWord* args, std::size_t argc) {
    return InvokeImpl(runtime, 0x0044C07Cu, args, argc);
}

ImplWord Stream__WriteLong(ImplRuntime& runtime, const ImplWord* args, std::size_t argc) {
    return InvokeImpl(runtime, 0x0044C41Cu, args, argc);
}

ImplWord Stream__WriteShort(ImplRuntime& runtime, const ImplWord* args, std::size_t argc) {
    return InvokeImpl(runtime, 0x0044BBE8u, args, argc);
}

ImplWord Stream__ReadLong(ImplRuntime& runtime, const ImplWord* args, std::size_t argc) {
    return InvokeImpl(runtime, 0x0044C094u, args, argc);
}

ImplWord Stream__ReadShort(ImplRuntime& runtime, const ImplWord* args, std::size_t argc) {
    return InvokeImpl(runtime, 0x0044C2ACu, args, argc);
}

ImplWord Stream__ReadBlock(ImplRuntime& runtime, const ImplWord* args, std::size_t argc) {
    return InvokeImpl(runtime, 0x0044C104u, args, argc);
}

ImplWord Stream__WriteBlock(ImplRuntime& runtime, const ImplWord* args, std::size_t argc) {
    return InvokeImpl(runtime, 0x0044B9D0u, args, argc);
}

ImplWord FlagManagerNoName__Reset(ImplRuntime& runtime, const ImplWord* args, std::size_t argc) {
    return InvokeImpl(runtime, 0x00202C24u, args, argc);
}

ImplWord Stream__ReadString(ImplRuntime& runtime, const ImplWord* args, std::size_t argc) {
    return InvokeImpl(runtime, 0x0044B94Cu, args, argc);
}

ImplWord Stream__WriteString(ImplRuntime& runtime, const ImplWord* args, std::size_t argc) {
    return InvokeImpl(runtime, 0x0044BC8Cu, args, argc);
}

ImplWord FlagManagerNoName__Deserialize(ImplRuntime& runtime, const ImplWord* args, std::size_t argc) {
    return InvokeImpl(runtime, 0x00202AF8u, args, argc);
}

ImplWord FlagManagerNoName__Serialize(ImplRuntime& runtime, const ImplWord* args, std::size_t argc) {
    return InvokeImpl(runtime, 0x00202C34u, args, argc);
}

ImplWord RandomSeed__Serialize(ImplRuntime& runtime, const ImplWord* args, std::size_t argc) {
    return InvokeImpl(runtime, 0x00507748u, args, argc);
}

ImplWord GameConfigData__Serialize(ImplRuntime& runtime, const ImplWord* args, std::size_t argc) {
    return InvokeImpl(runtime, 0x0050AEA0u, args, argc);
}

} // namespace fates::coreimpl
