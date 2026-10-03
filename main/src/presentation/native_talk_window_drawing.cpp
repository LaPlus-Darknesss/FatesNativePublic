#include "fates/presentation/native_talk_window_drawing.hpp"
#include <algorithm>
#include <bit>
#include <cmath>
#include <limits>
#include <map>

namespace fates::presentation::native {
using namespace runtime::native;using namespace io::native;using S=TalkWindowDrawStatus;
namespace {
constexpr std::uint32_t Draw=0x18f6c8;
std::int32_t Byte(std::uint32_t value){return std::bit_cast<std::int8_t>(static_cast<std::uint8_t>(value));}
std::int32_t Half(std::uint32_t value){return std::bit_cast<std::int16_t>(static_cast<std::uint16_t>(value));}
std::uint32_t Bits(float value){return std::bit_cast<std::uint32_t>(value);}
std::uint32_t Add(std::uint32_t a,std::uint32_t b){return Bits(std::bit_cast<float>(a)+std::bit_cast<float>(b));}
std::optional<std::int32_t> Integer(std::uint32_t bits) {
    const auto value=std::bit_cast<float>(bits);if(!std::isfinite(value) || value< -2147483648.f || value>=2147483648.f)return {};
    return static_cast<std::int32_t>(value);
}
}
struct NativeTalkWindowDrawing::State {
    struct Request {TalkWindowDrawObservation value;TalkWindowHandle window;std::uint32_t priority{};TalkWindowDrawInputs inputs;};
    std::weak_ptr<NativeProcessScheduler> scheduler;std::shared_ptr<NativeTalkWindow> windows;
    std::shared_ptr<NativeTalkDrawerDrawing> frames;std::shared_ptr<NativeCoordinateResources> coordinates;std::shared_ptr<NativeTalkLayout> layout;
    std::shared_ptr<NativeTalkStringDrawing> strings;std::shared_ptr<NativeTalkNamePlateDrawing> names;
    std::shared_ptr<NativeTalkDrawingStatics> statics;std::shared_ptr<PrimitiveDrawSink> sink;
    std::map<std::uint32_t,std::shared_ptr<Request>> requests;std::uint32_t serial{};
    bool Live() const {const auto s=scheduler.lock();return s && s->root(2);}
    S Mutable(ProcessAccess* access) const {const auto s=scheduler.lock();if(!s || !s->root(2))return S::Retired;
        if(access)return access->BelongsTo(*s)?S::Ready:S::MismatchedDomain;return s->busy()?S::Busy:S::Ready;}
    std::shared_ptr<Request> Get(TalkWindowDrawRequest id) const {if(!id)return {};const auto it=requests.find(id->serial);return it!=requests.end() && it->second->value.identity==id?it->second:nullptr;}
};
struct NativeTalkWindowDrawing::Continuation final:ProcessContinuation {
    enum class Stage {Start,Delta,FramePush,FrameBegin,FramePriority,FrameDepth,FrameEnd,Frame,WaitFrame,FramePop,
        Offset,WaitOffset,StringLayout,TrimLayout,TextPush,TextBegin,TextPriority,TextDepth,Scissor,ScissorEmit,Tev,TextEnd,
        String,WaitString,TextPop,NameGate,NameGuard,Name,WaitName,Done};
    std::shared_ptr<State> state;std::shared_ptr<State::Request> request;ProcessCall call;Stage stage{Stage::Start};
    std::optional<PrimitiveCommand> pending;UniqueArchiveRecord trim;std::array<std::uint32_t,2> string_xy{};
    std::array<std::int32_t,4> scissor{};std::uint8_t string_index{};
    ~Continuation() override {
        if(!request)return;auto& value=request->value;
        if(value.frame){state->frames->ForgetDraw(value.frame);value.frame.reset();}
        if(value.offset){state->statics->ForgetOffset(value.offset);value.offset.reset();}
        if(value.string){state->strings->ForgetDraw(value.string);value.string.reset();}
        if(value.name){state->names->ForgetDraw(value.name);value.name.reset();}
        if(!value.completed){value.active=false;value.cancelled=true;value.status=S::Cancelled;}
    }
    ProcessCallbackStep Block(S status=S::Unavailable){request->value.status=status;return ProcessCallbackStep::Blocked();}
    bool Submit(PrimitiveCommand command) {
        if(!pending)pending=std::move(command);if(state->sink && !state->sink->Submit(*pending))return false;
        ++request->value.commands;pending.reset();return true;
    }
    std::optional<std::int32_t> Read(const UniqueArchiveRecord& record,std::size_t field) const {
        const auto word=state->coordinates->ReadU16(record,field);return word.status==UniqueArchiveStatus::Ready?std::optional{Half(word.value)}:std::nullopt;
    }
    std::optional<std::int32_t> Dimension(std::size_t field) const {
        auto value=Read(trim,field);if(!value || *value>0)return value;
        const auto descriptor=state->coordinates->Pointer(trim,4);if(descriptor.status!=UniqueArchiveStatus::Ready)return {};
        if(!descriptor.value)return value;value=Read(descriptor.value,field);if(!value)return {};
        return Half(static_cast<std::uint32_t>(*value<0?-*value:*value));
    }
    ProcessCallbackStep Step(ProcessAccess& access) override {
        if(const auto status=state->Mutable(&access);status!=S::Ready)return Block(status);
        if(state->Get(request->value.identity)!=request)return Block(S::Cancelled);
        for(;;) {
            const auto window=state->windows->Observe(request->window);if(!window)return Block(S::InvalidWindow);
            auto& value=request->value;
            switch(stage) {
            case Stage::Start:
                value.initial_shout=Byte(window->next_icon);stage=*value.initial_shout<0?Stage::FramePush:Stage::Delta;break;
            case Stage::Delta: {
                if(!request->inputs.shout_delta)return Block();const auto delta=request->inputs.shout_delta();if(!delta)return Block();
                const auto next=static_cast<std::uint8_t>(std::min(*value.initial_shout+Byte(*delta),10));
                if(state->windows->WriteShoutIndex(request->window,next,access)!=TalkWindowStatus::Ready)return Block(S::InvalidWindow);
                value.written_shout=next;stage=Stage::FramePush;break;
            }
            case Stage::FramePush:if(!Submit(PrimitivePush{}))return Block();stage=Stage::FrameBegin;break;
            case Stage::FrameBegin:if(!Submit(PrimitiveBegin{}))return Block();stage=Stage::FramePriority;break;
            case Stage::FramePriority:if(!Submit(PrimitivePriority{static_cast<std::int16_t>(Half(request->priority))}))return Block();stage=Stage::FrameDepth;break;
            case Stage::FrameDepth:if(!Submit(PrimitiveDepth{0xbf000000}))return Block();stage=Stage::FrameEnd;break;
            case Stage::FrameEnd:if(!Submit(PrimitiveEnd{}))return Block();stage=Stage::Frame;break;
            case Stage::Frame: {
                if(!window->talk_type)return Block();const auto type=Byte(*window->talk_type);
                if(type<0 || type>2){stage=Stage::FramePop;break;}
                if(!value.frame) {
                    TalkDrawerFrameOptions options;options.next_icon=Byte(window->face_pending)!=0;
                    if(type!=2) {
                        if(!window->location)return Block();options.location=*window->location;options.shout_index=std::max(Byte(window->next_icon),-1);
                        // Original converts the unused pixel width too, before passing the frame arguments.
                        const auto width=Integer(window->offset_position[0]),x=Integer(window->position[0]),y=Integer(window->position[1]);
                        if(!width || !x || !y)return Block(S::InvalidGeometry);
                        options.xy={static_cast<std::uint32_t>(*x),static_cast<std::uint32_t>(*y)};
                    }
                    const auto windows=state->windows;const auto handle=request->window;const auto statics=state->statics;
                    TalkDrawerFrameInputs inputs{request->inputs.next_icon_suppressed,request->inputs.elapsed_ticks,[statics]{return statics->ReadPalette(0);}};
                    const auto color=[windows,handle]()->std::optional<FontDrawColor>{const auto row=windows->Observe(handle);return row?std::optional{row->frame_color}:std::nullopt;};
                    const auto entry=type==0?TalkDrawerDrawEntry::FaceWindow:type==1?TalkDrawerDrawEntry::StandWindow:TalkDrawerDrawEntry::SystemWindow;
                    if(state->frames->PrepareFrame(entry,options,std::move(inputs),color,value.frame,&access)!=TalkDrawerDrawStatus::Ready)return Block();
                }
                const auto child=state->frames->DrawCall(call.process,value.frame);if(!child)return Block();stage=Stage::WaitFrame;return ProcessCallbackStep::Call(*child);
            }
            case Stage::WaitFrame: {
                const auto child=state->frames->Observe(value.frame);if(!child || !child->completed)return Block();
                if(state->frames->Release(value.frame,&access)!=TalkDrawerDrawStatus::Ready)return Block();value.frame.reset();stage=Stage::FramePop;break;
            }
            case Stage::FramePop:if(!Submit(PrimitivePop{}))return Block();stage=Stage::Offset;break;
            case Stage::Offset: {
                if(!value.offset && state->statics->PrepareOffset(Byte(window->next_icon),value.offset,&access)!=TalkDrawingStaticStatus::Ready)return Block();
                const auto child=state->statics->OffsetCall(call.process,value.offset);if(!child)return Block();stage=Stage::WaitOffset;return ProcessCallbackStep::Call(*child);
            }
            case Stage::WaitOffset: {
                const auto child=state->statics->Observe(value.offset);if(!child || !child->completed || !child->result)return Block();value.text_offset=child->result;
                if(state->statics->Release(value.offset,&access)!=TalkDrawingStaticStatus::Ready)return Block();value.offset.reset();stage=Stage::StringLayout;break;
            }
            case Stage::StringLayout: {
                if(!window->location)return Block();const auto found=state->layout->Find(TalkLayoutItem::String,*window->location);
                if(found.status!=UniqueArchiveStatus::Ready || !found.value)return Block(S::InvalidRecord);
                const auto x=Read(found.value,8),y=Read(found.value,10);if(!x || !y)return Block(S::InvalidRecord);
                string_xy={Bits(static_cast<float>(*x)),Bits(static_cast<float>(*y))};stage=Stage::TrimLayout;break;
            }
            case Stage::TrimLayout: {
                if(!window->location)return Block();const auto found=state->layout->Find(TalkLayoutItem::StringTrim,*window->location);
                if(found.status!=UniqueArchiveStatus::Ready && found.status!=UniqueArchiveStatus::Missing)return Block(S::InvalidRecord);
                trim=found.value;stage=Stage::TextPush;break;
            }
            case Stage::TextPush:if(!Submit(PrimitivePush{}))return Block();stage=Stage::TextBegin;break;
            case Stage::TextBegin:if(!Submit(PrimitiveBegin{}))return Block();stage=Stage::TextPriority;break;
            case Stage::TextPriority:if(!Submit(PrimitivePriority{static_cast<std::int16_t>(Half(request->priority+1u))}))return Block();stage=Stage::TextDepth;break;
            case Stage::TextDepth:if(!Submit(PrimitiveDepth{0xbf000000}))return Block();stage=Stage::Scissor;break;
            case Stage::Scissor: {
                if(!trim){stage=Stage::Tev;break;}
                const auto height=Dimension(14),width=Dimension(12),y=Read(trim,10),x=Read(trim,8);
                if(!height || !width || !x || !y)return Block(S::InvalidRecord);
                const auto shout_x=(*value.text_offset)[0];
                const auto clip_y=Integer(Add(Add(window->position[1],shout_x),Bits(static_cast<float>(*y))));
                const auto clip_x=Integer(Add(Add(window->position[0],shout_x),Bits(static_cast<float>(*x))));
                if(!clip_x || !clip_y)return Block(S::InvalidGeometry);
                scissor={*clip_x,*clip_y,*width,*height};trim={};stage=Stage::ScissorEmit;break;
            }
            case Stage::ScissorEmit:if(!Submit(PrimitiveScissor{scissor}))return Block();stage=Stage::Tev;break;
            case Stage::Tev:if(!Submit(PrimitiveTevOp{2}))return Block();stage=Stage::TextEnd;break;
            case Stage::TextEnd:if(!Submit(PrimitiveEnd{}))return Block();stage=Stage::String;break;
            case Stage::String: {
                if(!value.string) {
                    if(!window->location)return Block();const auto& offset=*value.text_offset;
                    const TalkVectorBits argument{Add(Add(window->position[0],string_xy[0]),offset[0]),Add(Add(window->position[1],string_xy[1]),offset[1]),Add(Add(window->position[2],0),offset[2])};
                    const auto alpha=window->frame_color[3];const auto location=*window->location;
                    if(state->strings->Prepare({request->window,string_index},[argument]{return argument;},alpha,location,value.string,&access)!=TalkStringDrawStatus::Ready)return Block();
                    value.string_calls.push_back({string_index,argument,alpha,location});
                }
                const auto child=state->strings->DrawCall(call.process,value.string);if(!child)return Block();stage=Stage::WaitString;return ProcessCallbackStep::Call(*child);
            }
            case Stage::WaitString: {
                const auto child=state->strings->Observe(value.string);if(!child || !child->completed)return Block();
                if(state->strings->Release(value.string,&access)!=TalkStringDrawStatus::Ready)return Block();value.string.reset();
                stage=++string_index<8?Stage::String:Stage::TextPop;break;
            }
            case Stage::TextPop:if(!Submit(PrimitivePop{}))return Block();stage=Stage::NameGate;break;
            case Stage::NameGate:
                if(!window->talk_type)return Block();stage=*window->talk_type==2 || !window->strings_visible?Stage::Done:Stage::NameGuard;break;
            case Stage::NameGuard:
                if(state->statics->EnsureZeroVector(access)!=TalkDrawingStaticStatus::Ready)return Block();stage=Stage::Name;break;
            case Stage::Name: {
                if(!value.name) {
                    if(!window->location)return Block();const auto statics=state->statics;const auto priority=static_cast<std::uint16_t>(request->priority+80u);
                    if(state->names->Prepare(TalkNamePlateDrawEntry::Location,request->window,*window->location,priority,
                        [statics](std::size_t i){return statics->ReadZeroWord(i);},value.name,&access)!=TalkNamePlateDrawStatus::Ready)return Block();
                    value.name_called=true;value.name_location=*window->location;value.name_priority=priority;
                }
                const auto child=state->names->DrawCall(call.process,value.name);if(!child)return Block();stage=Stage::WaitName;return ProcessCallbackStep::Call(*child);
            }
            case Stage::WaitName: {
                const auto child=state->names->Observe(value.name);if(!child || !child->completed)return Block();
                if(state->names->Release(value.name,&access)!=TalkNamePlateDrawStatus::Ready)return Block();value.name.reset();stage=Stage::Done;break;
            }
            case Stage::Done:value.active=false;value.completed=true;value.status=S::Ready;return ProcessCallbackStep::Return();
            }
        }
    }
};
NativeTalkWindowDrawing::NativeTalkWindowDrawing(std::shared_ptr<State> state):state_(std::move(state)){}
NativeTalkWindowDrawing::~NativeTalkWindowDrawing()=default;
S NativeTalkWindowDrawing::Create(std::shared_ptr<NativeProcessScheduler> scheduler,std::shared_ptr<ProcessCallbackRegistry> registry,
    std::shared_ptr<NativeTalkWindow> windows,std::shared_ptr<NativeTalkDrawerDrawing> frames,std::shared_ptr<NativeCoordinateResources> coordinates,
    std::shared_ptr<NativeTalkLayout> layout,std::shared_ptr<NativeFontMetrics> metrics,std::shared_ptr<NativeFontDrawing> fonts,
    std::shared_ptr<NativeTalkStringDrawing> strings,std::shared_ptr<NativeTalkNamePlateDrawing> names,std::shared_ptr<NativeTalkDrawingStatics> statics,
    std::shared_ptr<PrimitiveDrawSink> sink,std::shared_ptr<NativeTalkWindowDrawing>& out) {
    if(!scheduler || !scheduler->root(2))return S::NullScheduler;
    if(!registry || !scheduler->UsesCallbacks(registry.get()) || !windows || !frames || !coordinates || !layout || !metrics || !fonts || !strings || !names || !statics ||
       !windows->UsesScheduler(*scheduler) || !frames->UsesScheduler(*scheduler) || !coordinates->UsesScheduler(*scheduler) || !layout->UsesCoordinates(*coordinates) ||
       !frames->UsesComposition(*coordinates,*layout,sink.get()) || !frames->UsesMetrics(*metrics) || !fonts->UsesMetrics(*metrics) ||
       !strings->UsesScheduler(*scheduler) || !strings->UsesComposition(*windows,*fonts) || !names->UsesScheduler(*scheduler) ||
       !names->UsesComposition(*windows,*frames,*coordinates,*layout,*metrics,*fonts,sink.get()) || !statics->UsesScheduler(*scheduler))return S::MismatchedDomain;
    auto state=std::make_shared<State>();state->scheduler=scheduler;state->windows=std::move(windows);state->frames=std::move(frames);state->coordinates=std::move(coordinates);
    state->layout=std::move(layout);state->strings=std::move(strings);state->names=std::move(names);state->statics=std::move(statics);state->sink=std::move(sink);
    auto next=std::shared_ptr<NativeTalkWindowDrawing>(new NativeTalkWindowDrawing(state));const std::array targets{Draw};if(!registry->Register(targets,next))return S::DuplicateBinding;
    out=std::move(next);return S::Ready;
}
S NativeTalkWindowDrawing::Prepare(TalkWindowHandle window,std::uint32_t priority,TalkWindowDrawInputs inputs,TalkWindowDrawRequest& out,ProcessAccess* access) {
    if(const auto status=state_->Mutable(access);status!=S::Ready)return status;if(!state_->windows->Observe(window))return S::InvalidWindow;
    if(state_->serial==std::numeric_limits<std::uint32_t>::max())return S::IdentityExhausted;
    auto row=std::make_shared<State::Request>();row->window=std::move(window);row->priority=priority;row->inputs=std::move(inputs);row->value.identity=std::make_shared<TalkWindowDrawIdentity>(++state_->serial);
    state_->requests.emplace(state_->serial,row);out=row->value.identity;return S::Ready;
}
std::optional<ProcessCall> NativeTalkWindowDrawing::DrawCall(ProcessHandle process,TalkWindowDrawRequest id) const {
    const auto row=state_->Get(id);if(!state_->Live() || !row || row->value.active || row->value.completed || row->value.cancelled)return {};
    ProcessCall call;call.process=std::move(process);call.kind=ProcessCallKind::Service;call.target=Draw;call.arguments[0]=id->serial;call.argument_count=1;return call;
}
std::optional<TalkWindowDrawObservation> NativeTalkWindowDrawing::Observe(TalkWindowDrawRequest id) const {const auto row=state_->Get(id);return state_->Live() && row?std::optional{row->value}:std::nullopt;}
S NativeTalkWindowDrawing::Release(TalkWindowDrawRequest id,ProcessAccess* access) {
    if(const auto status=state_->Mutable(access);status!=S::Ready)return status;const auto row=state_->Get(id);if(!row)return S::InvalidRequest;if(row->value.active)return S::Busy;
    state_->requests.erase(id->serial);return S::Ready;
}
bool NativeTalkWindowDrawing::UsesScheduler(const NativeProcessScheduler& scheduler) const noexcept{return state_->scheduler.lock().get()==&scheduler;}
std::unique_ptr<ProcessContinuation> NativeTalkWindowDrawing::Begin(const ProcessCall& call) {
    if(!state_->Live() || !call.process || call.kind!=ProcessCallKind::Service || call.has_self || call.this_adjustment || call.target!=Draw || call.argument_count!=1)return {};
    const auto it=state_->requests.find(call.arguments[0]);if(it==state_->requests.end())return {};const auto row=it->second;
    if(row->value.active || row->value.completed || row->value.cancelled)return {};auto next=std::make_unique<Continuation>();next->state=state_;next->request=row;next->call=call;row->value.active=true;return next;
}
}
