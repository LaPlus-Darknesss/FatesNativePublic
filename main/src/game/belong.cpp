#include "fates/game/belong.hpp"

#include "fates/detail/core_game_data_runtime.hpp"
#include "fates/engine/ident_hash.hpp"

#include <cstddef>

namespace {

Belong* gAffiliations = nullptr;
int gAffiliationCount = 0;

} // namespace

void Belong::Initialize(const void* data, int count) {
    gAffiliations = const_cast<Belong*>(
        static_cast<const Belong*>(data));
    gAffiliationCount = count;
}

Belong* Belong::Get(const char* identifier) {
    return static_cast<Belong*>(
        fates::decomp_detail::gAffiliationIdentHash->GetSurely(identifier));
}

Belong* Belong::Get(int affiliationId) {
    return reinterpret_cast<Belong*>(
        reinterpret_cast<std::byte*>(gAffiliations) +
        static_cast<std::ptrdiff_t>(affiliationId) * 0x10);
}

void Belong::Finalize() {
    if (gAffiliations != nullptr) {
        gAffiliationCount = 0;
        gAffiliations = nullptr;
    }
}
