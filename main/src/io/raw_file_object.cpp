#include "fates/io/raw_file_object.hpp"

#include "fates/detail/raw_file_runtime.hpp"

void RawFileObject::Setup() {}

void RawFileObject::Cleanup() {}

IAllocator* RawFileObject::GetAllocator() const {
    return fates::decomp_detail::GetRawFileAllocator();
}

bool RawFileObject::IsDelay() const {
    return false;
}

unsigned int RawFileObject::GetAlign() const {
    return 0x80;
}
