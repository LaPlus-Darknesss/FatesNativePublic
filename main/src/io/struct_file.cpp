#include "fates/io/struct_file.hpp"

#include "fates/detail/tex_struct_file_runtime.hpp"
#include "fates/io/file_object.hpp"
#include "fates/io/unique_archive_object.hpp"

StructFile::~StructFile() {
    Free();
}

void* StructFile::TryGet(const char* identifier) const {
    FileObject* const object = GetFileObject();
    if (object == nullptr) {
        return nullptr;
    }

    const UniqueArchiveObject* const archive =
        fates::decomp_detail::GetStructArchiveObject(*object);
    return archive != nullptr ? archive->TryGet(identifier) : nullptr;
}

void* StructFile::GetHeader(const char* identifier) const {
    FileObject* const object = GetFileObject();
    if (object == nullptr) {
        return nullptr;
    }

    const UniqueArchiveObject* const archive =
        fates::decomp_detail::GetStructArchiveObject(*object);
    return archive != nullptr ? archive->GetSurely(identifier) : nullptr;
}

void* StructFile::GetSurely(const char* identifier) const {
    FileObject* const object = GetFileObject();
    if (object == nullptr) {
        return nullptr;
    }

    const UniqueArchiveObject* const archive =
        fates::decomp_detail::GetStructArchiveObject(*object);
    return archive != nullptr ? archive->GetSurely(identifier) : nullptr;
}
