#include "fates/game/item_kinds.hpp"

#include <cstddef>

namespace {

ItemKind* gItemKinds = nullptr;
ItemSubKind* gItemSubKinds = nullptr;
ItemRefine* gItemRefineEntries = nullptr;

} // namespace

void ItemKind::Initialize(const void* data) {
    gItemKinds = const_cast<ItemKind*>(
        static_cast<const ItemKind*>(data));
}

ItemKind* ItemKind::Get(int kind) {
    return reinterpret_cast<ItemKind*>(
        reinterpret_cast<std::byte*>(gItemKinds) +
        static_cast<std::ptrdiff_t>(kind) * 0x0C);
}

void ItemKind::Finalize() {
    if (gItemKinds != nullptr) {
        gItemKinds = nullptr;
    }
}

void ItemSubKind::Initialize(const void* data) {
    gItemSubKinds = const_cast<ItemSubKind*>(
        static_cast<const ItemSubKind*>(data));
}

ItemSubKind* ItemSubKind::Get(int subKind) {
    return reinterpret_cast<ItemSubKind*>(
        reinterpret_cast<std::byte*>(gItemSubKinds) +
        static_cast<std::ptrdiff_t>(subKind) * 0x10);
}

void ItemSubKind::Finalize() {
    if (gItemSubKinds != nullptr) {
        gItemSubKinds = nullptr;
    }
}

void ItemRefine::Initialize(const void* data) {
    gItemRefineEntries = const_cast<ItemRefine*>(
        static_cast<const ItemRefine*>(data));
}

ItemRefine* ItemRefine::Get(int levelTable, int upgradeIndex) {
    return reinterpret_cast<ItemRefine*>(
        reinterpret_cast<std::byte*>(gItemRefineEntries) +
        static_cast<std::ptrdiff_t>(upgradeIndex + levelTable * 10) *
            0x04);
}

void ItemRefine::Finalize() {
    if (gItemRefineEntries != nullptr) {
        gItemRefineEntries = nullptr;
    }
}
