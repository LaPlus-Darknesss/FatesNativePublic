#pragma once

#include <cstddef>

class IdentHashEntry;

namespace fates::decomp_detail {

// Retail IdentHash instances share an entry-pool object. The pool itself is
// not yet promoted because its allocator/lifetime owner is outside the current
// closure, but the constructor's relationship to it is first-party ARM evidence.
struct IdentHashEntryPool {
    std::byte unknown_00[0x04]{};
    IdentHashEntry* freeList{};
};

IdentHashEntryPool* GetGlobalIdentHashEntryPool();
void DestroyIdentHashObject(class IdentHash* hash);

} // namespace fates::decomp_detail
