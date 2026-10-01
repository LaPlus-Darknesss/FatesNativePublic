#pragma once

class IAllocator;

namespace fates::decomp_detail {

// Retail RawFileObject::GetAllocator reads the allocator pointer held by the
// file-runtime owner. The containing owner is still part of the FileController
// frontier and remains opaque in this pass.
IAllocator* GetRawFileAllocator();

} // namespace fates::decomp_detail
