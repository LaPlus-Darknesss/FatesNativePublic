#include "fates/presentation/native_talk_name_plate_drawing.hpp"
#include <bit>
#include <cmath>
#include <limits>
#include <map>

namespace fates::presentation::native {
using namespace runtime::native;
using namespace io::native;
using S=TalkNamePlateDrawStatus;
using E=TalkNamePlateDrawEntry;
namespace {
constexpr std::uint32_t Location=0x1cb834,Absolute=0x1cb97c,Depth=0xbefae148;
std::uint32_t Target(E entry){return entry==E::Location?Location:Absolute;}
std::int32_t Signed16(std::uint32_t value){return std::bit_cast<std::int16_t>(static_cast<std::uint16_t>(value));}
std::int32_t HalfAbs(std::int32_t value){return Signed16(static_cast<std::uint32_t>(value<0?-value:value))/2;}
std::uint32_t Bits(float value){return std::bit_cast<std::uint32_t>(value);}
std::optional<std::uint32_t> Integer(std::uint32_t bits) {
    const float value=std::bit_cast<float>(bits);
    // Only the proven finite signed-conversion domain is admitted. Never invoke
    // undefined host casts or wrap unsupported VFP saturation inputs.
    if(!std::isfinite(value) || value < -2147483648.f || value >= 2147483648.f)return {};
    return std::bit_cast<std::uint32_t>(static_cast<std::int32_t>(value));
}
}
struct NativeTalkNamePlateDrawing::State {
    struct Request {
        TalkNamePlateDrawObservation value;E entry{};TalkWindowHandle window;
        std::uint32_t location{},priority{};PositionReader argument;
    };
    std::weak_ptr<NativeProcessScheduler> scheduler;std::shared_ptr<NativeTalkWindow> windows;
    std::shared_ptr<NativeTalkDrawerDrawing> frames;std::shared_ptr<NativeCoordinateResources> coordinates;
    std::shared_ptr<NativeTalkLayout> layout;std::shared_ptr<NativeFontMetrics> metrics;
    std::shared_ptr<NativeFontDrawing> fonts;std::shared_ptr<PrimitiveDrawSink> sink;
    std::map<std::uint32_t,std::shared_ptr<Request>> requests;std::uint32_t serial{};
    bool Live() const {const auto owner=scheduler.lock();return owner && owner->root(2);}
    S Mutable(ProcessAccess* access) const {
        const auto owner=scheduler.lock();if(!owner || !owner->root(2))return S::Retired;
        if(access)return access->BelongsTo(*owner)?S::Ready:S::MismatchedDomain;
        return owner->busy()?S::Busy:S::Ready;
    }
    std::shared_ptr<Request> Get(TalkNamePlateDrawRequest id) const {
        if(!id)return {};const auto found=requests.find(id->serial);
        return found!=requests.end() && found->second->value.identity==id?found->second:nullptr;
    }
};
struct NativeTalkNamePlateDrawing::Continuation final:ProcessContinuation {
    enum class Stage {Name,Push,Begin,Priority,DepthValue,End,FrameXY,Frame,WaitFrame,FramePop,
        FontPush,FontSelect,TextGroup,WidthName,Width,WaitWidth,Height,WaitHeight,Layout,Geometry,
        DrawName,DrawZ,Draw,WaitDraw,TextPop,FontPop,Done};
    std::shared_ptr<State> state;std::shared_ptr<State::Request> request;ProcessCall call;
    Stage stage{Stage::Name};bool text_group{};std::optional<PrimitiveCommand> pending;
    std::array<std::optional<std::uint32_t>,2> frame_input;std::array<std::uint32_t,2> frame_xy{};
    UniqueArchiveRecord position,descriptor;std::optional<TalkNameView> borrowed;
    std::uint32_t plate_z{};TalkVectorBits text_position{};
    ~Continuation() override {
        if(!request)return;
        if(request->value.frame){state->frames->ForgetDraw(request->value.frame);request->value.frame.reset();}
        if(request->value.measurement){state->metrics->ForgetText(request->value.measurement);request->value.measurement.reset();}
        if(request->value.glyphs){state->fonts->ForgetDraw(request->value.glyphs);request->value.glyphs.reset();}
        if(!request->value.completed){request->value.active=false;request->value.cancelled=true;request->value.status=S::Cancelled;}
    }
    ProcessCallbackStep Block(S status=S::Unavailable){request->value.status=status;return ProcessCallbackStep::Blocked();}
    bool Submit(PrimitiveCommand command) {
        if(!pending)pending=std::move(command);
        if(state->sink && !state->sink->Submit(*pending))return false;
        ++request->value.commands;pending.reset();return true;
    }
    void ReadName() {
        borrowed=state->windows->NameView(request->window);
        request->value.name_reads.push_back(borrowed?borrowed->allocation:TalkNameHandle{});
    }
    NativeFontMetrics::Reader Reader() const {
        const auto windows=state->windows;const auto view=borrowed;
        return [windows,view](std::size_t index)->std::optional<char16_t>{
            // A later null pointer is not the initial null early return. Keep
            // the actual child's read barrier and never turn it into empty text.
            if(!view)return {};
            TalkWindowSource source;source.kind=TalkWindowSource::Kind::Name;source.name_view=*view;
            const auto word=windows->Read(source,index);
            return word.status==TalkWindowStatus::Ready?std::optional{word.value}:std::nullopt;
        };
    }
    NativeFontDrawing::ColorReader Color(std::size_t index) const {
        const auto windows=state->windows;const auto window=request->window;
        return [windows,window,index]()->std::optional<FontDrawColor>{
            const auto row=windows->Observe(window);if(!row)return {};
            return windows->ReadColor(row->name_plate.color_members[index].identity);
        };
    }
    std::optional<std::int32_t> Read(const UniqueArchiveRecord& record,std::size_t offset) const {
        const auto row=state->coordinates->ReadU16(record,offset);
        return row.status==UniqueArchiveStatus::Ready?std::optional{Signed16(row.value)}:std::nullopt;
    }
    bool Geometry() {
        const auto tw=Read(descriptor,12),th=Read(descriptor,14);if(!tw || !th)return false;
        const auto plate=state->windows->Observe(request->window);if(!plate)return false;
        const auto half_width=std::bit_cast<std::int32_t>(*request->value.width)/2;
        const auto half_height=std::bit_cast<std::int32_t>(*request->value.height)/2;
        if(request->entry==E::Location) {
            const auto x=Read(position,8),y=Read(position,10);if(!x || !y)return false;
            const auto ix=static_cast<std::uint32_t>(*x)+static_cast<std::uint32_t>(HalfAbs(*tw))-static_cast<std::uint32_t>(half_width);
            const auto iy=static_cast<std::uint32_t>(*y)+static_cast<std::uint32_t>(HalfAbs(*th))-static_cast<std::uint32_t>(half_height)-1u;
            text_position[0]=Bits(static_cast<float>(std::bit_cast<std::int32_t>(ix)));
            text_position[1]=Bits(static_cast<float>(std::bit_cast<std::int32_t>(iy)));
            plate_z=plate->name_plate.position[2];
        } else {
            // Preserve the two separate binary32 operations, including their
            // order. Frame's earlier integer-truncated xy are not reused here.
            const float y=static_cast<float>(HalfAbs(*th))+std::bit_cast<float>(plate->name_plate.position[1]);
            text_position[1]=Bits(y-static_cast<float>(half_height));
            const float x=static_cast<float>(HalfAbs(*tw))+std::bit_cast<float>(plate->name_plate.position[0]);
            text_position[0]=Bits(x-static_cast<float>(half_width));
        }
        return true;
    }
    ProcessCallbackStep Step(ProcessAccess& access) override {
        if(const auto status=state->Mutable(&access);status!=S::Ready)return Block(status);
        if(state->Get(request->value.identity)!=request)return Block(S::Cancelled);
        for(;;) {
            if(!state->windows->Observe(request->window))return Block(S::InvalidWindow);
            switch(stage) {
            case Stage::Name:ReadName();stage=borrowed?Stage::Push:Stage::Done;break;
            case Stage::Push:if(!Submit(PrimitivePush{}))return Block();stage=Stage::Begin;break;
            case Stage::Begin:if(!Submit(PrimitiveBegin{}))return Block();stage=Stage::Priority;break;
            case Stage::Priority:
                if(!Submit(PrimitivePriority{static_cast<std::int16_t>(Signed16(request->priority+(text_group?1u:0u)))}))return Block();
                stage=Stage::DepthValue;break;
            case Stage::DepthValue:if(!Submit(PrimitiveDepth{Depth}))return Block();stage=Stage::End;break;
            case Stage::End:if(!Submit(PrimitiveEnd{}))return Block();stage=text_group?Stage::WidthName:Stage::FrameXY;break;
            case Stage::FrameXY: {
                for(std::size_t i=0;i<2;++i) {
                    if(!frame_input[i]) {
                        if(request->entry==E::Location)frame_input[i]=request->argument(i);
                        else frame_input[i]=state->windows->Observe(request->window)->name_plate.position[i];
                    }
                    if(!frame_input[i])return Block();const auto value=Integer(*frame_input[i]);if(!value)return Block(S::InvalidPosition);
                    frame_xy[i]=*value;
                }
                stage=Stage::Frame;break;
            }
            case Stage::Frame: {
                if(!request->value.frame) {
                    const auto entry=request->entry==E::Location?TalkDrawerDrawEntry::LocationNamePlate:TalkDrawerDrawEntry::AbsoluteNamePlate;
                    if(state->frames->Prepare(entry,request->location,frame_xy,Color(0),request->value.frame,&access)!=TalkDrawerDrawStatus::Ready)return Block();
                }
                const auto child=state->frames->DrawCall(call.process,request->value.frame);if(!child)return Block();
                stage=Stage::WaitFrame;return ProcessCallbackStep::Call(*child);
            }
            case Stage::WaitFrame: {
                const auto child=state->frames->Observe(request->value.frame);if(!child || !child->completed)return Block();
                if(state->frames->Release(request->value.frame,&access)!=TalkDrawerDrawStatus::Ready)return Block();
                request->value.frame.reset();stage=Stage::FramePop;break;
            }
            case Stage::FramePop:if(!Submit(PrimitivePop{}))return Block();stage=Stage::FontPush;break;
            case Stage::FontPush:stage=Stage::FontSelect;return ProcessCallbackStep::Call(NativeFontMetrics::PushCall(call.process));
            case Stage::FontSelect:stage=Stage::TextGroup;return ProcessCallbackStep::Call(NativeFontMetrics::SetCurrentCall(call.process,0));
            case Stage::TextGroup:text_group=true;stage=Stage::Push;break;
            case Stage::WidthName:ReadName();stage=Stage::Width;break;
            case Stage::Width: {
                if(!request->value.measurement && state->metrics->PrepareText(Reader(),FontTextOperation::Width,request->value.measurement,&access)!=FontMetricStatus::Ready)return Block();
                const auto child=state->metrics->TextCall(call.process,request->value.measurement);if(!child)return Block();
                stage=Stage::WaitWidth;return ProcessCallbackStep::Call(*child);
            }
            case Stage::WaitWidth: {
                const auto child=state->metrics->Observe(request->value.measurement);if(!child || !child->completed || !child->result)return Block();
                request->value.width=*child->result;
                if(state->metrics->Release(request->value.measurement,&access)!=FontMetricStatus::Ready)return Block();
                request->value.measurement.reset();stage=Stage::Height;break;
            }
            case Stage::Height:stage=Stage::WaitHeight;return ProcessCallbackStep::Call(NativeFontMetrics::MaxHeightCall(call.process));
            case Stage::WaitHeight:request->value.height=access.call_result();stage=Stage::Layout;break;
            case Stage::Layout: {
                if(request->entry==E::Location) {
                    const auto found=state->layout->Find(TalkLayoutItem::NamePlate,request->location);
                    if(found.status!=UniqueArchiveStatus::Ready || !found.value)return Block(S::InvalidRecord);
                    position=found.value;const auto texture=state->coordinates->Pointer(position,4);
                    if(texture.status!=UniqueArchiveStatus::Ready || !texture.value)return Block(S::InvalidRecord);descriptor=texture.value;
                } else {
                    const auto found=state->coordinates->Find("TalkW2_Mini2NameFrame");
                    if(found.status!=UniqueArchiveStatus::Ready || !found.value)return Block(S::InvalidRecord);descriptor=found.value;
                }
                stage=Stage::Geometry;break;
            }
            case Stage::Geometry:if(!Geometry())return Block(S::InvalidRecord);stage=Stage::DrawName;break;
            case Stage::DrawName:ReadName();stage=Stage::DrawZ;break;
            case Stage::DrawZ: {
                if(request->entry==E::Location) {
                    const auto value=request->argument(2);if(!value)return Block();
                    text_position[2]=Bits(std::bit_cast<float>(*value)+std::bit_cast<float>(plate_z));
                } else text_position[2]=state->windows->Observe(request->window)->name_plate.position[2];
                request->value.drawn_position=text_position;stage=Stage::Draw;break;
            }
            case Stage::Draw: {
                if(!request->value.glyphs) {
                    const auto status=state->fonts->Prepare(FontDrawEntry::Font3D,{},Reader(),Color(1),text_position,request->value.glyphs,&access);
                    if(status!=FontDrawStatus::Ready)return Block(status==FontDrawStatus::InvalidData?S::InvalidPosition:S::Unavailable);
                }
                const auto child=state->fonts->DrawCall(call.process,request->value.glyphs);if(!child)return Block();
                stage=Stage::WaitDraw;return ProcessCallbackStep::Call(*child);
            }
            case Stage::WaitDraw: {
                const auto child=state->fonts->Observe(request->value.glyphs);if(!child || !child->completed)return Block();
                if(state->fonts->Release(request->value.glyphs,&access)!=FontDrawStatus::Ready)return Block();
                request->value.glyphs.reset();stage=Stage::TextPop;break;
            }
            case Stage::TextPop:if(!Submit(PrimitivePop{}))return Block();stage=Stage::FontPop;break;
            case Stage::FontPop:stage=Stage::Done;return ProcessCallbackStep::Call(NativeFontMetrics::PopCall(call.process));
            case Stage::Done:
                request->value.active=false;request->value.completed=true;request->value.status=S::Ready;return ProcessCallbackStep::Return();
            }
        }
    }
};
NativeTalkNamePlateDrawing::NativeTalkNamePlateDrawing(std::shared_ptr<State> state):state_(std::move(state)){}
NativeTalkNamePlateDrawing::~NativeTalkNamePlateDrawing()=default;
S NativeTalkNamePlateDrawing::Create(std::shared_ptr<NativeProcessScheduler> scheduler,std::shared_ptr<ProcessCallbackRegistry> registry,
    std::shared_ptr<NativeTalkWindow> windows,std::shared_ptr<NativeTalkDrawerDrawing> frames,std::shared_ptr<NativeCoordinateResources> coordinates,
    std::shared_ptr<NativeTalkLayout> layout,std::shared_ptr<NativeFontMetrics> metrics,std::shared_ptr<NativeFontDrawing> fonts,
    std::shared_ptr<NativeFontPrimitiveAdapter> adapter,std::shared_ptr<PrimitiveDrawSink> sink,std::shared_ptr<NativeTalkNamePlateDrawing>& out) {
    if(!scheduler || !scheduler->root(2))return S::NullScheduler;
    if(!registry || !scheduler->UsesCallbacks(registry.get()) || !windows || !frames || !coordinates || !layout || !metrics || !fonts || !adapter ||
       !windows->UsesScheduler(*scheduler) || !frames->UsesScheduler(*scheduler) || !frames->UsesComposition(*coordinates,*layout,sink.get()) ||
       !metrics->UsesScheduler(*scheduler) || !fonts->UsesScheduler(*scheduler) || !fonts->UsesMetrics(*metrics) ||
       !fonts->UsesSink(adapter.get()) || !adapter->UsesSink(sink.get()))return S::MismatchedDomain;
    auto state=std::make_shared<State>();state->scheduler=scheduler;state->windows=std::move(windows);state->frames=std::move(frames);
    state->coordinates=std::move(coordinates);state->layout=std::move(layout);state->metrics=std::move(metrics);state->fonts=std::move(fonts);state->sink=std::move(sink);
    auto next=std::shared_ptr<NativeTalkNamePlateDrawing>(new NativeTalkNamePlateDrawing(state));
    const std::array targets{Location,Absolute};if(!registry->Register(targets,next))return S::DuplicateBinding;
    out=std::move(next);return S::Ready;
}
S NativeTalkNamePlateDrawing::Prepare(E entry,TalkWindowHandle window,std::uint32_t location,std::uint32_t priority,
    PositionReader argument,TalkNamePlateDrawRequest& out,ProcessAccess* access) {
    if(const auto status=state_->Mutable(access);status!=S::Ready)return status;
    if(entry!=E::Location && entry!=E::Absolute)return S::InvalidRequest;
    if(!state_->windows->Observe(window))return S::InvalidWindow;
    if(entry==E::Location && !argument)return S::InvalidRequest;
    if(state_->serial==std::numeric_limits<std::uint32_t>::max())return S::IdentityExhausted;
    auto request=std::make_shared<State::Request>();request->value.identity=std::make_shared<TalkNamePlateDrawIdentity>(++state_->serial);
    request->entry=entry;request->window=std::move(window);request->location=location;request->priority=priority;request->argument=std::move(argument);
    state_->requests.emplace(state_->serial,request);out=request->value.identity;return S::Ready;
}
std::optional<ProcessCall> NativeTalkNamePlateDrawing::DrawCall(ProcessHandle process,TalkNamePlateDrawRequest identity) const {
    const auto request=state_->Get(identity);
    if(!state_->Live() || !request || request->value.active || request->value.completed || request->value.cancelled)return {};
    ProcessCall call;call.process=std::move(process);call.kind=ProcessCallKind::Service;call.target=Target(request->entry);
    call.arguments[0]=identity->serial;call.argument_count=1;return call;
}
std::optional<TalkNamePlateDrawObservation> NativeTalkNamePlateDrawing::Observe(TalkNamePlateDrawRequest identity) const {
    const auto request=state_->Get(identity);return state_->Live() && request?std::optional{request->value}:std::nullopt;
}
S NativeTalkNamePlateDrawing::Release(TalkNamePlateDrawRequest identity,ProcessAccess* access) {
    if(const auto status=state_->Mutable(access);status!=S::Ready)return status;
    const auto request=state_->Get(identity);if(!request)return S::InvalidRequest;if(request->value.active)return S::Busy;
    state_->requests.erase(identity->serial);return S::Ready;
}
bool NativeTalkNamePlateDrawing::UsesScheduler(const NativeProcessScheduler& scheduler) const noexcept{return state_->scheduler.lock().get()==&scheduler;}
bool NativeTalkNamePlateDrawing::UsesComposition(const NativeTalkWindow& windows,const NativeTalkDrawerDrawing& frames,const NativeCoordinateResources& coordinates,
    const NativeTalkLayout& layout,const NativeFontMetrics& metrics,const NativeFontDrawing& fonts,const PrimitiveDrawSink* sink) const noexcept {
    return state_->windows.get()==&windows && state_->frames.get()==&frames && state_->coordinates.get()==&coordinates && state_->layout.get()==&layout &&
        state_->metrics.get()==&metrics && state_->fonts.get()==&fonts && state_->sink.get()==sink;
}
void NativeTalkNamePlateDrawing::ForgetDraw(TalkNamePlateDrawRequest id) {
    const auto row=state_->Get(id);if(!row)return;
    if(row->value.frame)state_->frames->ForgetDraw(row->value.frame);
    if(row->value.measurement)state_->metrics->ForgetText(row->value.measurement);
    if(row->value.glyphs)state_->fonts->ForgetDraw(row->value.glyphs);state_->requests.erase(id->serial);
}
std::unique_ptr<ProcessContinuation> NativeTalkNamePlateDrawing::Begin(const ProcessCall& call) {
    if(!state_->Live() || !call.process || call.kind!=ProcessCallKind::Service || call.has_self || call.this_adjustment || call.argument_count!=1 ||
       (call.target!=Location && call.target!=Absolute))return {};
    const auto found=state_->requests.find(call.arguments[0]);if(found==state_->requests.end())return {};
    const auto request=found->second;if(call.target!=Target(request->entry) || request->value.active || request->value.completed || request->value.cancelled)return {};
    auto next=std::make_unique<Continuation>();next->state=state_;next->request=request;next->call=call;request->value.active=true;return next;
}
}
