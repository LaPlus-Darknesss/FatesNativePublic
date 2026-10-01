#include "fates/leafcore/core_leaf_impl.hpp"

namespace fates::leafcore {

// PresentationLeaf: exact retail identity owned; unresolved layout/backend state remains behind LeafRuntime.

LeafWord Font__Draw_3(LeafRuntime& runtime, const LeafWord* args, std::size_t argc) {
    return InvokeLeaf(runtime, 0x003CFB38u, args, argc);
}

LeafWord UIFont__GetWidth(LeafRuntime& runtime, const LeafWord* args, std::size_t argc) {
    return InvokeLeaf(runtime, 0x0044D160u, args, argc);
}

LeafWord Color8__operator_4(LeafRuntime& runtime, const LeafWord* args, std::size_t argc) {
    return InvokeLeaf(runtime, 0x0053D5F8u, args, argc);
}

LeafWord GameFontHelper__EquipSkill__Draw(LeafRuntime& runtime, const LeafWord* args, std::size_t argc) {
    return InvokeLeaf(runtime, 0x001D088Cu, args, argc);
}

LeafWord GameFontHelper__Food__Draw(LeafRuntime& runtime, const LeafWord* args, std::size_t argc) {
    return InvokeLeaf(runtime, 0x001D0A98u, args, argc);
}

LeafWord GameFontHelper__Item__DrawForSignal(LeafRuntime& runtime, const LeafWord* args, std::size_t argc) {
    return InvokeLeaf(runtime, 0x001D0C14u, args, argc);
}

LeafWord GameFontHelper__Gemstone__Draw(LeafRuntime& runtime, const LeafWord* args, std::size_t argc) {
    return InvokeLeaf(runtime, 0x001D1294u, args, argc);
}

LeafWord GameFont__GetMaxWidth(LeafRuntime& runtime, const LeafWord* args, std::size_t argc) {
    return InvokeLeaf(runtime, 0x003CFA64u, args, argc);
}

LeafWord Icon__System__Draw_2(LeafRuntime& runtime, const LeafWord* args, std::size_t argc) {
    return InvokeLeaf(runtime, 0x003D0ADCu, args, argc);
}

LeafWord UIFont__GetMaxHeight(LeafRuntime& runtime, const LeafWord* args, std::size_t argc) {
    return InvokeLeaf(runtime, 0x0044C9B0u, args, argc);
}

LeafWord UIFont__Draw_2(LeafRuntime& runtime, const LeafWord* args, std::size_t argc) {
    return InvokeLeaf(runtime, 0x0044D084u, args, argc);
}

LeafWord Graphics__GetClearType(LeafRuntime& runtime, const LeafWord* args, std::size_t argc) {
    return InvokeLeaf(runtime, 0x004EBB54u, args, argc);
}

LeafWord Graphics__SetClearType(LeafRuntime& runtime, const LeafWord* args, std::size_t argc) {
    return InvokeLeaf(runtime, 0x004EBB64u, args, argc);
}

LeafWord ISFont__GetGlyphHeight(LeafRuntime& runtime, const LeafWord* args, std::size_t argc) {
    return InvokeLeaf(runtime, 0x0053D720u, args, argc);
}

LeafWord ISFont__GetGlyph_2(LeafRuntime& runtime, const LeafWord* args, std::size_t argc) {
    return InvokeLeaf(runtime, 0x0053D748u, args, argc);
}

} // namespace fates::leafcore
