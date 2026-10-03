#include "fates/presentation/native_primitive_drawing.hpp"

namespace fates::presentation::native {
NativeFontPrimitiveAdapter::NativeFontPrimitiveAdapter(std::shared_ptr<PrimitiveDrawSink> sink):sink_(std::move(sink)){}
bool NativeFontPrimitiveAdapter::UsesSink(const PrimitiveDrawSink* sink) const noexcept{return sink_.get()==sink;}
bool NativeFontPrimitiveAdapter::Submit(const FontDrawCommand& source) {
    PrimitiveCommand command;
    switch(source.kind) {
    case FontDrawCommandKind::Push:command=PrimitivePush{};break;
    case FontDrawCommandKind::Begin:command=PrimitiveBegin{};break;
    case FontDrawCommandKind::RectShape:command=PrimitiveRectShape{};break;
    case FontDrawCommandKind::Texture:
        if(!source.sheet)return false;
        command=PrimitiveTexture{*source.sheet};break;
    case FontDrawCommandKind::End:command=PrimitiveEnd{};break;
    case FontDrawCommandKind::Rectangle:
        if(!source.sheet)return false;
        command=PrimitiveFontRectangle{source.values,source.color,*source.sheet};break;
    case FontDrawCommandKind::Pop:command=PrimitivePop{};break;
    default:return false;
    }
    return !sink_ || sink_->Submit(command);
}
}
