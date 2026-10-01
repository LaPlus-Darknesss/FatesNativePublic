#include "fates/io/native_file_store.hpp"
#include <limits>

namespace fates::io::native {
namespace {
FileStoreStatus Translate(FileSourceStatus s) {
    switch(s) {
    case FileSourceStatus::Ready:return FileStoreStatus::Ready;
    case FileSourceStatus::Missing:return FileStoreStatus::Missing;
    default:return FileStoreStatus::SourceUnavailable;
    }
}
}
FileStoreStatus NativeFileStore::Validate(std::string_view path) const noexcept {
    if(retired_)return FileStoreStatus::Retired;
    if(path.size()>1024 || path.find('\0')!=std::string_view::npos)return FileStoreStatus::InvalidPath;
    return FileStoreStatus::Ready;
}
FileStoreStatus NativeFileStore::Exists(std::string_view path) const {
    const auto s=Validate(path);if(s!=FileStoreStatus::Ready)return s;
    return source_.exists?Translate(source_.exists(path)):FileStoreStatus::SourceUnavailable;
}
FileStoreResult NativeFileStore::Loaded(std::string_view path) const {
    const auto s=Validate(path);if(s!=FileStoreStatus::Ready)return {s,{}};
    const std::string key(path);const auto* entry=entries_.Find(key.c_str());
    return entry?FileStoreResult{FileStoreStatus::Ready,entry->value->image}:FileStoreResult{};
}
FileStoreResult NativeFileStore::Acquire(std::string_view path) {
    using S=FileStoreStatus;
    const auto s=Validate(path);if(s!=S::Ready)return {s,{}};
    const std::string key(path);
    if(const auto* entry=entries_.Find(key.c_str())) {
        if(entry->value->references==std::numeric_limits<std::uint32_t>::max())return {S::ReferenceOverflow,{}};
        ++entry->value->references;return {S::Ready,entry->value->image};
    }
    if(entries_.Size()>=65536)return {S::LimitExceeded,{}};
    if(!source_.read)return {S::SourceUnavailable,{}};
    auto image=std::shared_ptr<NativeFileImage>(new NativeFileImage);image->path_=key;
    const auto read=Translate(source_.read(path,image->bytes_));if(read!=S::Ready)return {read,{}};
    auto entry=std::make_shared<Entry>();entry->image=image;
    entries_.Append(key.c_str(),std::move(entry));return {S::Ready,std::move(image)};
}
FileStoreStatus NativeFileStore::Release(std::string_view path) {
    const auto s=Validate(path);if(s!=FileStoreStatus::Ready)return s;
    const std::string key(path);const auto* entry=entries_.Find(key.c_str());
    if(!entry)return FileStoreStatus::Missing;
    if(--entry->value->references==0) {
        // Retail removes by the stored file's name, not the caller's spelling.
        const std::string stored(entry->value->image->path());entries_.EraseFirst(stored.c_str());
    }
    return FileStoreStatus::Ready;
}
std::uint32_t NativeFileStore::references(std::string_view path) const {
    if(Validate(path)!=FileStoreStatus::Ready)return 0;
    const std::string key(path);const auto* entry=entries_.Find(key.c_str());
    return entry?entry->value->references:0;
}
}
