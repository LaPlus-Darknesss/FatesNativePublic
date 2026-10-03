#pragma once
#include "fates/runtime/native_archive.hpp"
#include "fates/io/native_file_store.hpp"
#include <map>

namespace fates::runtime::native {
enum class ArchiveIdentifierStatus : std::uint8_t {
    Ready,Missing,Retired,InvalidImage,InvalidArchive,InvalidIdentifier,
    ForeignRegistration,StaleValue,LimitExceeded
};
class NativeArchiveIdentifiers;
// Optional concrete data-owner lifetime. Standalone immutable archive fixtures
// need no lease; a live FileObject association supplies one so retained bytes
// cannot make a destroyed/replaced game resource appear live.
class ArchiveImageLifetime {
public:
    virtual ~ArchiveImageLifetime()=default;
    virtual bool live() const noexcept=0;
};
class ArchiveRegistration final {
public:
    bool registered() const noexcept {return registered_;}
    const std::shared_ptr<const io::native::NativeFileImage>& image() const noexcept {return image_;}
private:
    friend class NativeArchiveIdentifiers;
    std::shared_ptr<const std::uint8_t> domain_;
    std::shared_ptr<const io::native::NativeFileImage> image_;
    ArchiveLabelTable labels_;
    std::shared_ptr<const ArchiveImageLifetime> lifetime_;
    bool registered_{};
};
struct ArchiveIdentifierValue {
    std::shared_ptr<const ArchiveRegistration> registration;
    std::size_t payload_offset{};
};
struct ArchiveIdentifierResult {
    ArchiveIdentifierStatus status{ArchiveIdentifierStatus::Missing};
    ArchiveIdentifierValue value;
};
struct ArchiveIdentifierWordResult {
    ArchiveIdentifierStatus status{ArchiveIdentifierStatus::Missing};
    std::uint32_t value{};
};
struct ArchiveIdentifierStringResult {
    ArchiveIdentifierStatus status{ArchiveIdentifierStatus::Missing};
    // Ready + nullopt is an actual null pointer; Ready + empty is nonnull.
    std::optional<std::string> text;
};
// Shared Ident::Initialize index, not a message-only dictionary. Empty creation
// explicitly owns a newly initialized registry; it does not import or infer the
// existing game's global membership. Archive values are retained image-relative
// identities. Typed consumers still need their concrete data/relocation owner.
// Entry-pool allocation is a host boundary; no full original allocator claim.
class NativeArchiveIdentifiers final {
public:
    NativeArchiveIdentifiers()=default;
    NativeArchiveIdentifiers(const NativeArchiveIdentifiers&)=delete;
    NativeArchiveIdentifiers& operator=(const NativeArchiveIdentifiers&)=delete;
    ~NativeArchiveIdentifiers() {Retire();}
    ArchiveIdentifierStatus Construct(std::shared_ptr<const io::native::NativeFileImage>,
        std::shared_ptr<const ArchiveRegistration>&,std::shared_ptr<const ArchiveImageLifetime> ={});
    ArchiveIdentifierStatus Destruct(const std::shared_ptr<const ArchiveRegistration>&);
    ArchiveIdentifierStatus DestructImage(const std::shared_ptr<const io::native::NativeFileImage>&,
        const std::shared_ptr<const ArchiveImageLifetime>&);
    ArchiveIdentifierResult Find(std::string_view) const;
    // Read the first scalar word at the actual first-hash-match value. Never
    // redo a spelling lookup in its archive. Relocated pointer fields and
    // one-past-data label identities are not scalar words.
    ArchiveIdentifierWordResult ReadWord(std::string_view) const;
    // Project a pointer field at the returned value identity. Relocation fields
    // are applied semantically, never as host pointers. Targets may be in the
    // archive's trailing string area. Null without relocation stays null;
    // relative zero WITH relocation points to the data base.
    ArchiveIdentifierStringResult ReadStringField(const ArchiveIdentifierValue&,std::size_t field) const;
    // Bounded raw UTF16 source observation from a retained value. Unlike text
    // length, this can expose admitted words after NUL to original token readers.
    // Relocated pointer bytes are never projected as scalar UTF16.
    ArchiveIdentifierWordResult ReadHalfword(const ArchiveIdentifierValue&,std::size_t displacement) const;
    std::size_t size() const noexcept {return index_.Size();}
    std::uint32_t accounting_count() const noexcept {return index_.AccountingCount();}
    void Retire() noexcept;
private:
    std::shared_ptr<const std::uint8_t> domain_=std::make_shared<const std::uint8_t>(std::uint8_t{0});
    IdentifierRegistry<ArchiveIdentifierValue> index_{2039};
    std::map<const io::native::NativeFileImage*,std::shared_ptr<ArchiveRegistration>> active_;
    // ArchiveConstruct's flag belongs to the image, including a surviving value
    // from first-match deletion. Reconstructing that same retained image must
    // reuse its identity; weak history does not keep unused images alive.
    std::map<const io::native::NativeFileImage*,std::weak_ptr<ArchiveRegistration>> known_;
    bool retired_{};
};
}
