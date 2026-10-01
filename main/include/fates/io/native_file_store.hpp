#pragma once
#include "fates/runtime/native_identifier.hpp"
#include <functional>
#include <memory>
#include <span>

namespace fates::io::native {
enum class FileSourceStatus : std::uint8_t { Ready, Missing, Unavailable };
// Platform transport only. Read supplies FileBase's final, immutable file bytes;
// filesystem mounting, asynchronous I/O and decompression are outside this seam.
// Calls are synchronous and must not reenter or mutate the store/session.
struct NativeFileSource {
    std::function<FileSourceStatus(std::string_view)> exists;
    std::function<FileSourceStatus(std::string_view,std::vector<std::uint8_t>&)> read;
};
class NativeFileImage final {
public:
    std::span<const std::uint8_t> bytes() const noexcept {return bytes_;}
    std::string_view path() const noexcept {return path_;}
private:
    friend class NativeFileStore;
    std::string path_;
    std::vector<std::uint8_t> bytes_;
};
enum class FileStoreStatus : std::uint8_t {
    Ready, Missing, SourceUnavailable, InvalidPath, Retired, LimitExceeded, ReferenceOverflow
};
struct FileStoreResult {
    FileStoreStatus status{FileStoreStatus::Missing};
    std::shared_ptr<const NativeFileImage> image;
};
// Native GlobalFile file-registry projection. Existence in the source and
// membership in this retained registry are different operations. Reuse the
// original 127-bucket, first-hash-match index, including spelling collisions.
// Acquisitions are counted; external immutable observations do not count as
// GlobalFile references and cannot revive a released registry entry.
class NativeFileStore final {
public:
    explicit NativeFileStore(NativeFileSource source):source_(std::move(source)) {}
    NativeFileStore(const NativeFileStore&)=delete;
    NativeFileStore& operator=(const NativeFileStore&)=delete;
    FileStoreStatus Exists(std::string_view) const;
    FileStoreResult Acquire(std::string_view);
    FileStoreResult Loaded(std::string_view) const;
    FileStoreStatus Release(std::string_view);
    std::uint32_t references(std::string_view) const;
    std::size_t size() const noexcept {return entries_.Size();}
    void Retire() noexcept {retired_=true;entries_.Clear();}
    bool retired() const noexcept {return retired_;}
private:
    struct Entry {std::shared_ptr<const NativeFileImage> image;std::uint32_t references{1};};
    FileStoreStatus Validate(std::string_view) const noexcept;
    NativeFileSource source_;
    runtime::native::IdentifierRegistry<std::shared_ptr<Entry>> entries_{127};
    bool retired_{};
};
}
