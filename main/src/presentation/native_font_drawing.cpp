#include "fates/presentation/native_font_drawing.hpp"
#include <algorithm>
#include <bit>
#include <cmath>
#include <limits>
#include <map>

namespace fates::presentation::native {
using namespace runtime::native;
using namespace io::native;
using S=FontDrawStatus;
namespace {
constexpr std::array<std::uint32_t,3> Targets{0x5061e0,0x3cfab4,0x3cfb38};
constexpr std::size_t ReadBound=1u<<20;
std::uint16_t U16(const std::array<std::uint8_t,16>& bytes,std::size_t at) {
    return static_cast<std::uint16_t>(std::uint16_t(bytes[at])|(std::uint16_t(bytes[at+1])<<8));
}
std::uint32_t Bits(float value){return std::bit_cast<std::uint32_t>(value);}
float Float(std::uint32_t value){return std::bit_cast<float>(value);}
}
struct NativeFontDrawing::State {
    struct Request {
        FontDrawObservation value;FontDrawEntry entry{};FileObjectHandle object;
        Reader reader;ColorReader color;std::array<std::uint32_t,3> position{};
    };
    std::weak_ptr<NativeProcessScheduler> scheduler;
    std::shared_ptr<NativeFontMetrics> metrics;std::shared_ptr<NativeFontObjects> objects;
    std::shared_ptr<NativeFontStartup> startup;std::shared_ptr<FontDrawSink> sink;
    std::map<std::uint32_t,std::shared_ptr<Request>> requests;std::uint32_t serial{};
    bool Live() const {const auto s=scheduler.lock();return s && s->root(2);}
    S Mutable(ProcessAccess* access) const {
        const auto s=scheduler.lock();if(!s || !s->root(2))return S::Retired;
        if(access)return access->BelongsTo(*s)?S::Ready:S::MismatchedDomain;
        return s->busy()?S::Busy:S::Ready;
    }
    std::shared_ptr<Request> Get(FontDrawRequest identity) const {
        if(!identity)return {};
        const auto found=requests.find(identity->serial);
        return found!=requests.end() && found->second->value.identity==identity?found->second:nullptr;
    }
};
struct NativeFontDrawing::Continuation final:ProcessContinuation {
    enum class Stage {Capture,Push,Header,Clear,BindBegin,BindShape,BindTexture,BindEnd,
        ReadTest,ReadGlyph,Lookup,Queue,Rect,Advance,ReadNext,EmitPage,EmitGlyph,Pop,Done};
    std::shared_ptr<State> state;std::shared_ptr<State::Request> request;
    Stage stage{Stage::Capture},after_bind{Stage::ReadTest},after_rect{Stage::Advance};
    FileObjectHandle object;std::optional<FontMetricObservation> header;
    std::optional<FontSheetView> sheet;std::optional<FontDrawCommand> pending;
    FontMetricView glyph;char16_t word{};std::uint32_t x{},glyph_x{},page{},clear_index{};
    std::int32_t advance{};std::size_t cursor{},emit_index{};
    ~Continuation() override {
        if(request && !request->value.completed){request->value.active=false;request->value.cancelled=true;request->value.status=S::Cancelled;}
    }
    ProcessCallbackStep Block(S status=S::Unavailable){request->value.status=status;return ProcessCallbackStep::Blocked();}
    bool Current() const {
        if(!header)return true;
        const auto now=state->metrics->Observe(object);
        return now && now->data_revision==header->data_revision && now->setup_generation==header->setup_generation;
    }
    bool ValidSheet() const {
        if(!sheet)return false;
        NativeTextureDescription description;
        return state->objects->Describe(*sheet,description)==FontObjectStatus::Ready;
    }
    bool Submit(FontDrawCommandKind kind) {
        if(!pending){pending=FontDrawCommand{};pending->kind=kind;if(kind==FontDrawCommandKind::Texture)pending->sheet=sheet;}
        if(state->sink && !state->sink->Submit(*pending))return false;
        ++request->value.commands;if(kind==FontDrawCommandKind::Rectangle)++request->value.rectangles;
        pending.reset();return true;
    }
    std::optional<char16_t> Read() {
        if(request->value.read_attempts>=ReadBound)return {};
        ++request->value.read_attempts;return request->reader(cursor);
    }
    ProcessCallbackStep Step(ProcessAccess& access) override {
        if(state->Mutable(&access)!=S::Ready)return Block(S::Retired);
        if(state->Get(request->value.identity)!=request)return Block(S::Cancelled);
        for(;;) {
            if(!Current())return Block(S::StaleData);
            switch(stage) {
            case Stage::Capture: {
                if(request->entry==FontDrawEntry::Object)object=request->object;
                else {const auto selection=state->metrics->Selection();if(!selection || !selection->current)return Block();object=*selection->current;}
                // Known-null selection has no retail early return. Keep an
                // explicit admission barrier rather than inventing empty text.
                if(!object)return Block(S::InvalidData);
                x=request->position[0];stage=Stage::Push;break;
            }
            case Stage::Push:
                if(!Submit(FontDrawCommandKind::Push))return Block();stage=Stage::Header;break;
            case Stage::Header:
                header=state->metrics->Observe(object);if(!header)return Block(S::StaleData);
                if(header->declared_sheets>8)return Block(S::InvalidData);
                stage=header->declared_sheets==1?Stage::BindBegin:Stage::Clear;break;
            case Stage::Clear:
                while(clear_index<header->declared_sheets) {
                    if(state->startup->ClearDrawBuffer(clear_index,access)!=FontStartupStatus::Ready)return Block();
                    ++clear_index;
                }
                stage=Stage::ReadTest;break;
            case Stage::BindBegin:
                if(!Submit(FontDrawCommandKind::Begin))return Block();stage=Stage::BindShape;break;
            case Stage::BindShape:
                if(!Submit(FontDrawCommandKind::RectShape))return Block();stage=Stage::BindTexture;break;
            case Stage::BindTexture:
                if(!sheet)sheet=state->objects->GetSheet(object,page);
                if(!ValidSheet())return Block(S::StaleData);
                if(!Submit(FontDrawCommandKind::Texture))return Block();stage=Stage::BindEnd;break;
            case Stage::BindEnd:
                if(!ValidSheet())return Block(S::StaleData);
                if(!Submit(FontDrawCommandKind::End))return Block();stage=after_bind;break;
            case Stage::ReadTest: {
                if(request->value.read_attempts>=ReadBound)return Block(S::ReadLimit);
                const auto read=Read();if(!read)return Block();word=*read;
                stage=word?Stage::ReadGlyph:(header->declared_sheets==1?Stage::Pop:Stage::EmitPage);break;
            }
            case Stage::ReadGlyph: {
                if(request->value.read_attempts>=ReadBound)return Block(S::ReadLimit);
                const auto read=Read();if(!read)return Block();word=*read;stage=Stage::Lookup;break;
            }
            case Stage::Lookup: {
                const auto result=state->metrics->GetGlyph(object,word,&access);
                if(result.status!=FontMetricStatus::Ready)return Block(S::StaleData);
                glyph=result.view;glyph_x=x;stage=header->declared_sheets==1?Stage::Rect:Stage::Queue;break;
            }
            case Stage::Queue: {
                FontGlyphRecord record;if(state->metrics->Describe(glyph,record)!=FontMetricStatus::Ready)return Block(S::StaleData);
                const auto index=U16(record.bytes,2);if(index>=8)return Block(S::InvalidData);
                bool stored=false;if(state->startup->AppendDrawGlyph(index,{glyph,x},stored,access)!=FontStartupStatus::Ready)return Block();
                if(!stored)++request->value.dropped_glyphs;
                stage=Stage::Advance;break;
            }
            case Stage::Rect: {
                FontGlyphRecord record;if(state->metrics->Describe(glyph,record)!=FontMetricStatus::Ready || !ValidSheet())return Block(S::StaleData);
                if(!pending) {
                    ++request->value.color_attempts;const auto color=request->color();if(!color)return Block();
                    FontDrawCommand command;command.kind=FontDrawCommandKind::Rectangle;command.sheet=sheet;command.color=*color;
                    const auto& bytes=record.bytes;
                    const float y=static_cast<float>(header->baseline_height)+Float(request->position[1]);
                    command.values={Bits(static_cast<float>(std::bit_cast<std::int8_t>(bytes[10]))+Float(glyph_x)),
                        Bits(y-static_cast<float>(std::bit_cast<std::int8_t>(bytes[11]))),request->position[2],
                        Bits(static_cast<float>(bytes[8])),Bits(static_cast<float>(bytes[9])),
                        Bits(static_cast<float>(U16(bytes,4))),Bits(static_cast<float>(U16(bytes,6)))};
                    pending=command;
                }
                // A host color provider may cross another owned resource
                // boundary. Revalidate after it, before publishing the view.
                if(!Current() || !ValidSheet())return Block(S::StaleData);
                if(!Submit(FontDrawCommandKind::Rectangle))return Block();
                if(after_rect==Stage::EmitGlyph)++emit_index;
                stage=after_rect;break;
            }
            case Stage::Advance: {
                FontGlyphRecord record;if(state->metrics->Describe(glyph,record)!=FontMetricStatus::Ready)return Block(S::StaleData);
                advance=record.advance;++cursor;stage=Stage::ReadNext;break;
            }
            case Stage::ReadNext: {
                if(request->value.read_attempts>=ReadBound)return Block(S::ReadLimit);
                const auto read=Read();if(!read)return Block();word=*read;
                // Original loads the next UTF16 word before adding the signed
                // advance. Preserve that order across reader suspension.
                x=Bits(Float(x)+static_cast<float>(advance));
                stage=word?Stage::ReadGlyph:(header->declared_sheets==1?Stage::Pop:Stage::EmitPage);break;
            }
            case Stage::EmitPage:
                if(page>=header->declared_sheets){stage=Stage::Pop;break;}
                if(const auto size=state->startup->DrawBufferSize(page);!size)return Block();
                else if(!*size){++page;break;}
                emit_index=0;sheet.reset();after_bind=Stage::EmitGlyph;after_rect=Stage::EmitGlyph;stage=Stage::BindBegin;break;
            case Stage::EmitGlyph: {
                const auto size=state->startup->DrawBufferSize(page);if(!size)return Block();
                if(emit_index==*size){++page;stage=Stage::EmitPage;break;}
                const auto item=state->startup->DrawGlyph(page,emit_index);if(!item)return Block(S::InvalidData);
                glyph=item->glyph;glyph_x=item->x_bits;stage=Stage::Rect;break;
            }
            case Stage::Pop:
                if(!Submit(FontDrawCommandKind::Pop))return Block();stage=Stage::Done;break;
            case Stage::Done:
                request->value.active=false;request->value.completed=true;request->value.status=S::Ready;return ProcessCallbackStep::Return();
            }
        }
    }
};
NativeFontDrawing::NativeFontDrawing(std::shared_ptr<State> state):state_(std::move(state)){}
NativeFontDrawing::~NativeFontDrawing()=default;
S NativeFontDrawing::Create(std::shared_ptr<NativeProcessScheduler> scheduler,std::shared_ptr<ProcessCallbackRegistry> registry,
    std::shared_ptr<NativeFontMetrics> metrics,std::shared_ptr<NativeFontObjects> objects,std::shared_ptr<NativeFontStartup> startup,
    std::shared_ptr<FontDrawSink> sink,std::shared_ptr<NativeFontDrawing>& out) {
    if(!scheduler || !scheduler->root(2))return S::NullScheduler;
    if(!registry || !scheduler->UsesCallbacks(registry.get()) || !metrics || !objects || !startup ||
       !metrics->UsesScheduler(*scheduler) || !objects->UsesScheduler(*scheduler) || !startup->UsesOwners(*metrics,*objects))return S::MismatchedDomain;
    auto state=std::make_shared<State>();state->scheduler=scheduler;state->metrics=std::move(metrics);
    state->objects=std::move(objects);state->startup=std::move(startup);state->sink=std::move(sink);
    auto next=std::shared_ptr<NativeFontDrawing>(new NativeFontDrawing(state));if(!registry->Register(Targets,next))return S::DuplicateBinding;
    out=std::move(next);return S::Ready;
}
S NativeFontDrawing::Prepare(FontDrawEntry entry,FileObjectHandle object,Reader reader,ColorReader color,
    std::array<std::uint32_t,3> position,FontDrawRequest& out,ProcessAccess* access) {
    if(const auto status=state_->Mutable(access);status!=S::Ready)return status;
    if(entry!=FontDrawEntry::Object && entry!=FontDrawEntry::Font2D && entry!=FontDrawEntry::Font3D)return S::InvalidRequest;
    if(!reader || !color || (entry==FontDrawEntry::Object && !object))return S::InvalidRequest;
    if(entry==FontDrawEntry::Font2D)position[2]=0;
    for(const auto value:position)if(!std::isfinite(Float(value)))return S::InvalidData;
    if(state_->serial==std::numeric_limits<std::uint32_t>::max())return S::IdentityExhausted;
    auto request=std::make_shared<State::Request>();request->value.identity=std::make_shared<FontDrawIdentity>(++state_->serial);
    request->entry=entry;request->object=std::move(object);request->reader=std::move(reader);request->color=std::move(color);request->position=position;
    state_->requests.emplace(request->value.identity->serial,request);out=request->value.identity;return S::Ready;
}
std::optional<ProcessCall> NativeFontDrawing::DrawCall(ProcessHandle process,FontDrawRequest identity) const {
    const auto request=state_->Get(identity);if(!state_->Live() || !request || request->value.active || request->value.completed || request->value.cancelled)return {};
    ProcessCall call;call.process=std::move(process);call.kind=ProcessCallKind::Service;
    call.target=Targets[static_cast<std::size_t>(request->entry)];call.arguments[0]=identity->serial;call.argument_count=1;return call;
}
std::optional<FontDrawObservation> NativeFontDrawing::Observe(FontDrawRequest identity) const {
    const auto request=state_->Get(identity);return state_->Live() && request?std::optional{request->value}:std::nullopt;
}
S NativeFontDrawing::Release(FontDrawRequest identity,ProcessAccess* access) {
    if(const auto status=state_->Mutable(access);status!=S::Ready)return status;
    const auto request=state_->Get(identity);if(!request)return S::InvalidRequest;
    if(request->value.active)return S::Busy;
    state_->requests.erase(identity->serial);return S::Ready;
}
bool NativeFontDrawing::UsesScheduler(const NativeProcessScheduler& scheduler) const noexcept{return state_->scheduler.lock().get()==&scheduler;}
bool NativeFontDrawing::UsesMetrics(const NativeFontMetrics& metrics) const noexcept{return state_->metrics.get()==&metrics;}
bool NativeFontDrawing::UsesSink(const FontDrawSink* sink) const noexcept{return state_->sink.get()==sink;}
void NativeFontDrawing::ForgetDraw(FontDrawRequest identity) noexcept {
    if(state_->Get(identity))state_->requests.erase(identity->serial);
}
std::unique_ptr<ProcessContinuation> NativeFontDrawing::Begin(const ProcessCall& call) {
    if(!state_->Live() || !call.process || call.has_self || call.this_adjustment || call.kind!=ProcessCallKind::Service || call.argument_count!=1)return {};
    const auto found=state_->requests.find(call.arguments[0]);if(found==state_->requests.end())return {};
    const auto request=found->second;
    if(call.target!=Targets[static_cast<std::size_t>(request->entry)] || request->value.active || request->value.completed || request->value.cancelled)return {};
    auto next=std::make_unique<Continuation>();next->state=state_;next->request=request;request->value.active=true;return next;
}
}
