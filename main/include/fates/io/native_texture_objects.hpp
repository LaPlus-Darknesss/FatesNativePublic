#pragma once
#include "fates/io/native_file_entry.hpp"
#include "fates/graphics/portable_texture.hpp"
#include <array>

namespace fates::io::native {
enum class TextureObjectStatus:std::uint8_t {
    Ready,Dummy,NullScheduler,MismatchedDomain,DuplicateBinding,Busy,Retired,
    InvalidObject,InvalidImage,ConflictingImage,UnsupportedResource,InvalidIndex,InvalidName,StaleValue
};
struct TextureStorage;
struct TextureGeneration;
// A retained native texture never grants an extra FileBase game reference.
// Cleanup, re-Setup, allocation replacement and deletion retire the view even
// when a client still retains its immutable bytes for diagnostic purposes.
class NativeTextureView {
public:
    explicit operator bool() const noexcept {return bool(generation_);}
    bool SameTexture(const NativeTextureView&) const noexcept;
private:
    friend class NativeTextureObjects;
    std::weak_ptr<TextureStorage> storage_;
    std::shared_ptr<const TextureGeneration> generation_;
    std::size_t index_{};
};
struct NativeTextureDescription {
    std::string name;
    std::uint16_t width{},height{};
    std::uint8_t format{},mip_count{},filter{},wrap{},mode{},target{};
    std::size_t source_offset{},packed_size{};
    // Original SetMatrix, retained as binary32 rather than host GPU state.
    std::array<float,12> matrix{};
};
// Vendor resource parsing is a renderer-neutral replacement boundary, just as
// NativeFieldWorld uses its external resource backend. It has no cache, game
// reference, process-readiness or resource-lifetime policy.
class NativeTextureResource {
public:
    virtual ~NativeTextureResource()=default;
    virtual std::span<const NativeTextureDescription> contents() const noexcept=0;
    virtual bool DecodeBase(std::size_t,graphics::portable::TextureImage&,std::string&) const=0;
};
struct NativeTextureResourceBackend {
    std::function<bool(std::span<const std::uint8_t>,std::shared_ptr<const NativeTextureResource>&,std::string&)> prepare;
};
// Original positive-dimension ITexture::GetImageSize binary32 order.
// Unsupported signed-product / conversion / sum overflow refuses before output.
bool ComputeNativeTextureImageSize(std::uint16_t,std::uint16_t,std::uint8_t,std::uint8_t,std::uint32_t&) noexcept;
struct TextureLookupResult {TextureObjectStatus status{TextureObjectStatus::InvalidObject};NativeTextureView value;};
struct TextureObjectObservation {
    std::uint32_t resource_flags{},texture_count{};
    bool resource_present{},texture_array_present{};
    std::size_t scratch_bytes{};
    std::string unresolved;
};
// Fates ResObject/TexObject lifecycle over the shared lower file allocation.
// The portable H3D replacement admits texture-only BCH resources with the
// reached inline-storage policy (low seven file flags == 2). Other placement,
// model/light linking and resource-relocation paths remain explicit barriers.
class NativeTextureObjects final:public runtime::native::ProcessCallbacks {
public:
    static TextureObjectStatus Create(std::shared_ptr<runtime::native::NativeProcessScheduler>,
        std::shared_ptr<runtime::native::ProcessCallbackRegistry>,std::shared_ptr<NativeFileController>,
        std::shared_ptr<NativeFileBase>,std::shared_ptr<NativeFileEntry>,NativeTextureResourceBackend,
        std::shared_ptr<NativeTextureObjects>&);
    ~NativeTextureObjects();
    NativeTextureObjects(const NativeTextureObjects&)=delete;
    NativeTextureObjects& operator=(const NativeTextureObjects&)=delete;
    TextureObjectStatus Construct(FileObjectHandle&,runtime::native::ProcessAccess* =nullptr);
    TextureObjectStatus PublishAllocator(std::uint32_t,runtime::native::ProcessAccess* =nullptr);
    TextureObjectStatus Associate(FileObjectHandle,std::shared_ptr<const NativeFileImage>,runtime::native::ProcessAccess* =nullptr);
    std::optional<TextureObjectObservation> Observe(FileObjectHandle) const;
    TextureLookupResult GetTexture(FileObjectHandle,std::int32_t) const;
    TextureLookupResult GetTexture(FileObjectHandle,std::optional<std::string_view>) const;
    TextureObjectStatus Describe(const NativeTextureView&,NativeTextureDescription&) const;
    TextureObjectStatus DecodeBase(const NativeTextureView&,graphics::portable::TextureImage&,std::string&) const;
    bool UsesOwners(const NativeFileController&,const NativeFileEntry&) const noexcept;
    bool UsesScheduler(const runtime::native::NativeProcessScheduler&) const noexcept;
    static FileObjectMethods Methods() noexcept;
    std::unique_ptr<runtime::native::ProcessContinuation> Begin(const runtime::native::ProcessCall&) override;
private:
    struct State;struct Continuation;
    explicit NativeTextureObjects(std::shared_ptr<State>);
    std::shared_ptr<State> state_;
};
}
