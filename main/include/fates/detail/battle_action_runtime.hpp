#pragma once
#include <utility>
namespace fates::decomp_detail {
template<class... A> inline void BattleActionCall(const char*, A&&...) {}
template<class T, class... A> inline T BattleActionValue(const char*, A&&...) { return T{}; }
}
