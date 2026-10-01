#pragma once
#include <utility>
namespace fates::decomp_detail {
template<class... A> inline void UnitOwnershipCall(const char*, A&&...) {}
template<class T,class... A> inline T UnitOwnershipValue(const char*, A&&...) { return T{}; }
}
