#pragma once
#include "fates/presentation/native_font_drawing.hpp"
#include "fates/io/native_texture_objects.hpp"
#include <variant>

namespace fates::presentation::native {
// Explicit original pointer categories. Unknown lookup is represented by the
// caller's optional, never by Null or the nonnull TexFile dummy singleton.
struct PrimitiveNullTexture {};
struct PrimitiveDummyTexture {};
using CoordinateTextureBinding=std::variant<PrimitiveNullTexture,PrimitiveDummyTexture,io::native::NativeTextureView>;
using PrimitiveTextureBinding=std::variant<PrimitiveNullTexture,PrimitiveDummyTexture,FontSheetView,io::native::NativeTextureView>;
struct PrimitivePush {};
struct PrimitiveBegin {};
struct PrimitiveRectShape {std::uint32_t value{};};
struct PrimitiveTexture {PrimitiveTextureBinding value;};
struct PrimitiveDepth {std::uint32_t bits{};};
struct PrimitiveEnd {};
struct PrimitiveFontRectangle {std::array<std::uint32_t,7> values{};FontDrawColor color{};FontSheetView sheet;};
struct PrimitiveUvRectangle {std::array<std::uint32_t,9> values{};FontDrawColor color{};};
struct PrimitiveFlatRectangle {std::array<std::uint32_t,5> values{};FontDrawColor color{};};
struct PrimitivePop {};
struct PrimitivePriority {std::int16_t value{};};
struct PrimitiveScissor {std::array<std::int32_t,4> values{};};
struct PrimitiveTevOp {std::uint32_t value{};};
using PrimitiveCommand=std::variant<PrimitivePush,PrimitiveBegin,PrimitiveRectShape,PrimitiveTexture,PrimitiveDepth,
    PrimitiveEnd,PrimitiveFontRectangle,PrimitiveUvRectangle,PrimitiveFlatRectangle,PrimitivePop,PrimitivePriority,PrimitiveScissor,PrimitiveTevOp>;
class PrimitiveDrawSink {
public:
    virtual ~PrimitiveDrawSink()=default;
    // False commits nothing. A retry supplies the same pending command.
    // Borrowed resource views must be resolved while accepting the command;
    // retained diagnostic bytes cannot revive a retired game allocation.
    virtual bool Submit(const PrimitiveCommand&)=0;
};
// Preserve the accepted seven-scalar font interface while sharing one ordered
// output stream with the distinct five- and nine-scalar coordinate overloads.
// The font owner remains responsible for validating its sheets before Submit.
class NativeFontPrimitiveAdapter final:public FontDrawSink {
public:
    explicit NativeFontPrimitiveAdapter(std::shared_ptr<PrimitiveDrawSink>);
    bool Submit(const FontDrawCommand&) override;
    bool UsesSink(const PrimitiveDrawSink*) const noexcept;
private:
    std::shared_ptr<PrimitiveDrawSink> sink_;
};
}
