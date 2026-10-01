#include "fates/io/unique_archive_object.hpp"

#include "fates/detail/ident_hash_runtime.hpp"
#include "fates/engine/ident_hash.hpp"
#include "fates/io/archive_index.hpp"

void UniqueArchiveObject::Setup() {
    index_ = new IdentHash(0x7F);
    ArchiveConstruct(archiveStorage_, index_);
}

void UniqueArchiveObject::Cleanup() {
    ArchiveDestruct(archiveStorage_, index_);
    if (index_ != nullptr) {
        fates::decomp_detail::DestroyIdentHashObject(index_);
        index_ = nullptr;
    }
}

void* UniqueArchiveObject::TryGet(const char* identifier) const {
    if (index_ == nullptr || identifier == nullptr) {
        return nullptr;
    }
    return index_->GetSurely(identifier);
}

void* UniqueArchiveObject::GetSurely(const char* identifier) const {
    // Retail release code performs the same hash lookup as TryGet; the
    // assertion/debug distinction is absent from the shipped body.
    return TryGet(identifier);
}
