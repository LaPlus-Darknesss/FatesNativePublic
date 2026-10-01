#include "fates/game/personality.hpp"

#include <cstddef>

namespace {

Personality* gPersonalities = nullptr;
int gPersonalityCount = 0;

} // namespace

void Personality::Initialize(const void* data, int count) {
    gPersonalities = const_cast<Personality*>(
        static_cast<const Personality*>(data));
    gPersonalityCount = count;
}

Personality* Personality::Get(int personalityId) {
    return reinterpret_cast<Personality*>(
        reinterpret_cast<std::byte*>(gPersonalities) +
        static_cast<std::ptrdiff_t>(personalityId) * 0x40);
}

void Personality::Finalize() {
    if (gPersonalities != nullptr) {
        gPersonalityCount = 0;
        gPersonalities = nullptr;
    }
}
