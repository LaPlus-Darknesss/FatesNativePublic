#pragma once
#include <type_traits>
#include <utility>

namespace fates::decomp_detail {

template <class... Args>
inline void MapBattleResolutionCall(const char*, Args&&... args) {
    (static_cast<void>(args), ...);
}

template <class R, class... Args>
inline R MapBattleResolutionValue(const char*, Args&&... args) {
    (static_cast<void>(args), ...);
    if constexpr (std::is_pointer_v<R>) return nullptr;
    else return R{};
}

} // namespace fates::decomp_detail
