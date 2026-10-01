#pragma once

#include "fates/detail/arm32_address.hpp"

#include <cstddef>
#include <cstdint>

namespace fates::decomp_detail {

constexpr std::size_t kPersonTableSupportOffset = 0x08;
constexpr std::size_t kSupportBucketCount = 0x80;

inline const void* GetPersonSupportTable(const void* personTable) {
    return TargetPointer(ReadArm32Address(personTable, kPersonTableSupportOffset));
}

// Human-facing terminology uses the official localized concept "Support".
// The exact retail C++ symbols are Reliance::* and stay in comments/evidence.
void InitializeSupportSystem();                 // Reliance::Initialize
void LoadSupportTable(const void* supportTable); // Reliance::Load
void FreeSupportTable(const void* supportTable); // Reliance::Free
void FinalizeSupportSystem();                   // Reliance::Finalize

} // namespace fates::decomp_detail
