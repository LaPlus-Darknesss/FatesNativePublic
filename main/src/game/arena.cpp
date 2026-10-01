#include "fates/game/arena.hpp"

namespace {
const ArenaTable* gLowArena = nullptr;
const ArenaTable* gHighArena = nullptr;
}

void Arena::Initialize(const void* data) {
    const auto* bytes = static_cast<const std::byte*>(data);
    gLowArena = reinterpret_cast<const ArenaTable*>(bytes + 0x00);
    gHighArena = reinterpret_cast<const ArenaTable*>(bytes + 0x08);
}

const ArenaTable* Arena::GetLow() {
    return gLowArena;
}

const ArenaTable* Arena::GetHigh() {
    return gHighArena;
}
