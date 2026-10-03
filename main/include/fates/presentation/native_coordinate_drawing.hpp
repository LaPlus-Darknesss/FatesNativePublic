#pragma once
#include "fates/presentation/native_primitive_drawing.hpp"
#include "fates/io/native_coordinate_resources.hpp"

namespace fates::presentation::native {
enum class CoordinateDrawStatus:std::uint8_t {
    Ready,NullScheduler,MismatchedDomain,DuplicateBinding,Busy,Retired,
    InvalidRequest,InvalidRecord,InvalidTexture,Unavailable,IdentityExhausted,Cancelled,InvalidGeometry
};
enum class CoordinateDrawEntry:std::uint8_t {Position,Rectangle,Scaled};
struct CoordinateDrawIdentity final {const std::uint32_t serial;};
using CoordinateDrawRequest=std::shared_ptr<const CoordinateDrawIdentity>;
struct CoordinateDrawObservation {
    CoordinateDrawRequest identity,child;CoordinateDrawStatus status{CoordinateDrawStatus::Ready};
    bool active{},completed{},cancelled{};
    std::size_t commands{},color_attempts{};
    std::optional<FontDrawColor> copied_color;
};
// TextureCoordinate::Position::Draw and DrawRect over live archive records.
// No second coordinate parser, texture cache, FileBase reference or GPU policy.
// Primitive submission is a renderer boundary, not a pixels-rendered claim.
class NativeCoordinateDrawing final:public runtime::native::ProcessCallbacks {
public:
    using ColorReader=NativeFontDrawing::ColorReader;
    static CoordinateDrawStatus Create(std::shared_ptr<runtime::native::NativeProcessScheduler>,
        std::shared_ptr<runtime::native::ProcessCallbackRegistry>,std::shared_ptr<io::native::NativeCoordinateResources>,
        std::shared_ptr<io::native::NativeTextureObjects>,std::shared_ptr<PrimitiveDrawSink>,std::shared_ptr<NativeCoordinateDrawing>&);
    ~NativeCoordinateDrawing();
    NativeCoordinateDrawing(const NativeCoordinateDrawing&)=delete;
    NativeCoordinateDrawing& operator=(const NativeCoordinateDrawing&)=delete;
    // xy are the original integer bits, not floats. Draw captures its texture
    // argument and copies color after setup. DrawRect ignores binding/depth.
    CoordinateDrawStatus Prepare(CoordinateDrawEntry,io::native::UniqueArchiveRecord,
        std::optional<CoordinateTextureBinding>,std::array<std::uint32_t,2>,ColorReader,bool,
        CoordinateDrawRequest&,runtime::native::ProcessAccess* =nullptr);
    // DrawScaledPrimTextureCoordinate: actual binary32 scale bits, two nested
    // primitive groups, centered geometry and final signed16 narrowing. A
    // missing descriptor is a reached barrier, never a flat-rectangle fallback.
    CoordinateDrawStatus PrepareScaled(io::native::UniqueArchiveRecord,
        std::optional<CoordinateTextureBinding>,std::array<std::uint32_t,2> xy,
        std::array<std::uint32_t,2> scale_bits,ColorReader,CoordinateDrawRequest&,
        runtime::native::ProcessAccess* =nullptr);
    std::optional<runtime::native::ProcessCall> DrawCall(runtime::native::ProcessHandle,CoordinateDrawRequest) const;
    std::optional<CoordinateDrawObservation> Observe(CoordinateDrawRequest) const;
    CoordinateDrawStatus Release(CoordinateDrawRequest,runtime::native::ProcessAccess* =nullptr);
    bool UsesScheduler(const runtime::native::NativeProcessScheduler&) const noexcept;
    bool UsesOwners(const io::native::NativeCoordinateResources&,const io::native::NativeTextureObjects&) const noexcept;
    bool UsesSink(const PrimitiveDrawSink*) const noexcept;
    std::unique_ptr<runtime::native::ProcessContinuation> Begin(const runtime::native::ProcessCall&) override;
private:
    friend class NativeTalkDrawerDrawing;
    void ForgetDraw(CoordinateDrawRequest);
    struct State;struct Continuation;
    explicit NativeCoordinateDrawing(std::shared_ptr<State>);
    std::shared_ptr<State> state_;
};
}
