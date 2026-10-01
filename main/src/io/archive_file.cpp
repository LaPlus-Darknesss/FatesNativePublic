#include "fates/io/archive_file.hpp"

#include "fates/detail/file_runtime.hpp"
#include "fates/io/raw_file_object.hpp"

ArchiveFile::ArchiveFile() = default;

ArchiveFile::ArchiveFile(const char* path) {
    const FileType fileType = OpenFile(path, ReadType::Immediate);
    if (fileType == FileType::Missing) {
        EntryFile(new RawFileObject(), path, 0, ReadType::Immediate);
    }
    (void)CloseFile(fileType, ReadType::Immediate);
}

ArchiveFile::~ArchiveFile() {
    Free();
}

namespace fates::decomp_detail {

ArchiveFile* OpenArchiveFile(const char* path) {
    return new ArchiveFile(path);
}

void DestroyArchiveFileOwner(ArchiveFile* archive) {
    delete archive;
}

} // namespace fates::decomp_detail
