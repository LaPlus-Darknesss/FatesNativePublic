#pragma once

namespace ism {
class HeapBase;
}

namespace fates::decomp_detail {

// Retail ExpandableUnitHeap::Initialize uses a process/global heap when the
// caller supplies no backing heap.  Pass 21 promotes the GlobalHeap facade,
// while selection of the process-owned singleton remains a runtime boundary.
ism::HeapBase* GetDefaultIsmHeap();

} // namespace fates::decomp_detail
