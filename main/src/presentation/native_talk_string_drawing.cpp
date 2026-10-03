#include "fates/presentation/native_talk_string_drawing.hpp"
#include <bit>
#include <limits>
#include <map>

namespace fates::presentation::native {
using namespace runtime::native;
using S=TalkStringDrawStatus;
namespace {constexpr std::uint32_t Draw=0x18e308;}
struct NativeTalkStringDrawing::State {
    struct Request {
        TalkStringDrawObservation value;TalkWindowView string;
        PositionReader position;std::uint32_t alpha_factor{};
    };
    std::weak_ptr<NativeProcessScheduler> scheduler;
    std::shared_ptr<NativeTalkWindow> windows;std::shared_ptr<NativeFontDrawing> fonts;
    std::map<std::uint32_t,std::shared_ptr<Request>> requests;std::uint32_t serial{};
    bool Live() const {const auto s=scheduler.lock();return s && s->root(2);}
    S Mutable(ProcessAccess* access) const {
        const auto s=scheduler.lock();if(!s || !s->root(2))return S::Retired;
        if(access)return access->BelongsTo(*s)?S::Ready:S::MismatchedDomain;
        return s->busy()?S::Busy:S::Ready;
    }
    std::shared_ptr<Request> Get(TalkStringDrawRequest identity) const {
        if(!identity)return {};
        const auto found=requests.find(identity->serial);
        return found!=requests.end() && found->second->value.identity==identity?found->second:nullptr;
    }
};
struct NativeTalkStringDrawing::Continuation final:ProcessContinuation {
    std::shared_ptr<State> state;std::shared_ptr<State::Request> request;ProcessCall call;
    unsigned stage{};std::optional<TalkVectorBits> argument;
    ~Continuation() override {
        if(!request)return;
        if(request->value.child){state->fonts->ForgetDraw(request->value.child);request->value.child.reset();}
        if(!request->value.completed){request->value.active=false;request->value.cancelled=true;request->value.status=S::Cancelled;}
    }
    ProcessCallbackStep Block(S status=S::Unavailable){request->value.status=status;return ProcessCallbackStep::Blocked();}
    ProcessCallbackStep Step(ProcessAccess& access) override {
        if(const auto status=state->Mutable(&access);status!=S::Ready)return Block(status);
        if(state->Get(request->value.identity)!=request)return Block(S::Cancelled);
        for(;;) {
            if(stage==0) {
                const auto string=state->windows->ObserveString(request->string);if(!string)return Block(S::InvalidWindow);
                request->value.copied_color=string->color;stage=1;
            }
            if(stage==1) {
                const auto string=state->windows->ObserveString(request->string);if(!string)return Block(S::InvalidWindow);
                // Original MUL wraps to32bits before its signed magic divide.
                // Conversion to the destination byte is modulo256, including
                // negative or out-of-range caller factors. No clamping.
                const auto product=std::uint32_t(string->color[3])*request->alpha_factor;
                const auto scaled=std::bit_cast<std::int32_t>(product)/255;
                (*request->value.copied_color)[3]=static_cast<std::uint8_t>(scaled);stage=2;
            }
            if(stage==2) {
                argument=request->position();if(!argument)return Block();stage=3;
            }
            if(stage==3) {
                const auto string=state->windows->ObserveString(request->string);if(!string)return Block(S::InvalidWindow);
                TalkVectorBits position;
                // Original computes z first, followed by x and y. Each add is
                // binary32; there is no integer truncation or shared transform.
                for(const unsigned i:{2u,0u,1u})position[i]=std::bit_cast<std::uint32_t>(
                    std::bit_cast<float>((*argument)[i])+std::bit_cast<float>(string->position[i]));
                request->value.drawn_position=position;stage=4;
            }
            if(stage==4) {
                TalkWindowSource source;source.kind=TalkWindowSource::Kind::WindowString;source.string_view=request->string;
                const auto windows=state->windows;const auto color=*request->value.copied_color;
                const auto reader=[windows,source](std::size_t at)->std::optional<char16_t>{
                    const auto word=windows->Read(source,at);return word.status==TalkWindowStatus::Ready?std::optional{word.value}:std::nullopt;
                };
                const auto status=state->fonts->Prepare(FontDrawEntry::Font3D,{},reader,[color]{return color;},
                    *request->value.drawn_position,request->value.child,&access);
                if(status!=FontDrawStatus::Ready)return Block(status==FontDrawStatus::InvalidData?S::InvalidPosition:S::Unavailable);
                stage=5;
            }
            if(stage==5) {
                const auto child=state->fonts->DrawCall(call.process,request->value.child);if(!child)return Block();
                stage=6;return ProcessCallbackStep::Call(*child);
            }
            const auto child=state->fonts->Observe(request->value.child);
            if(!child || !child->completed)return Block();
            if(state->fonts->Release(request->value.child,&access)!=FontDrawStatus::Ready)return Block();
            request->value.child.reset();request->value.active=false;request->value.completed=true;request->value.status=S::Ready;
            return ProcessCallbackStep::Return();
        }
    }
};
NativeTalkStringDrawing::NativeTalkStringDrawing(std::shared_ptr<State> state):state_(std::move(state)){}
NativeTalkStringDrawing::~NativeTalkStringDrawing()=default;
S NativeTalkStringDrawing::Create(std::shared_ptr<NativeProcessScheduler> scheduler,std::shared_ptr<ProcessCallbackRegistry> registry,
    std::shared_ptr<NativeTalkWindow> windows,std::shared_ptr<NativeFontDrawing> fonts,std::shared_ptr<NativeTalkStringDrawing>& out) {
    if(!scheduler || !scheduler->root(2))return S::NullScheduler;
    if(!registry || !scheduler->UsesCallbacks(registry.get()) || !windows || !fonts ||
       !windows->UsesScheduler(*scheduler) || !fonts->UsesScheduler(*scheduler))return S::MismatchedDomain;
    auto state=std::make_shared<State>();state->scheduler=scheduler;state->windows=std::move(windows);state->fonts=std::move(fonts);
    auto next=std::shared_ptr<NativeTalkStringDrawing>(new NativeTalkStringDrawing(state));
    const std::array targets{Draw};if(!registry->Register(targets,next))return S::DuplicateBinding;
    out=std::move(next);return S::Ready;
}
S NativeTalkStringDrawing::Prepare(TalkWindowView string,PositionReader position,std::uint32_t alpha_factor,
    std::uint32_t location,TalkStringDrawRequest& out,ProcessAccess* access) {
    if(const auto status=state_->Mutable(access);status!=S::Ready)return status;
    if(!state_->windows->ObserveString(string))return S::InvalidWindow;
    if(!position)return S::InvalidRequest;
    if(state_->serial==std::numeric_limits<std::uint32_t>::max())return S::IdentityExhausted;
    static_cast<void>(location); // Original r3 is overwritten before any read.
    auto request=std::make_shared<State::Request>();request->value.identity=std::make_shared<TalkStringDrawIdentity>(++state_->serial);
    request->string=std::move(string);request->position=std::move(position);request->alpha_factor=alpha_factor;
    state_->requests.emplace(request->value.identity->serial,request);out=request->value.identity;return S::Ready;
}
std::optional<ProcessCall> NativeTalkStringDrawing::DrawCall(ProcessHandle process,TalkStringDrawRequest identity) const {
    const auto request=state_->Get(identity);
    if(!state_->Live() || !request || request->value.active || request->value.completed || request->value.cancelled)return {};
    ProcessCall call;call.process=std::move(process);call.kind=ProcessCallKind::Service;call.target=Draw;call.arguments[0]=identity->serial;call.argument_count=1;return call;
}
std::optional<TalkStringDrawObservation> NativeTalkStringDrawing::Observe(TalkStringDrawRequest identity) const {
    const auto request=state_->Get(identity);return state_->Live() && request?std::optional{request->value}:std::nullopt;
}
S NativeTalkStringDrawing::Release(TalkStringDrawRequest identity,ProcessAccess* access) {
    if(const auto status=state_->Mutable(access);status!=S::Ready)return status;
    const auto request=state_->Get(identity);if(!request)return S::InvalidRequest;if(request->value.active)return S::Busy;
    state_->requests.erase(identity->serial);return S::Ready;
}
std::unique_ptr<ProcessContinuation> NativeTalkStringDrawing::Begin(const ProcessCall& call) {
    if(!state_->Live() || !call.process || call.kind!=ProcessCallKind::Service || call.has_self || call.this_adjustment || call.target!=Draw || call.argument_count!=1)return {};
    const auto found=state_->requests.find(call.arguments[0]);if(found==state_->requests.end())return {};
    const auto request=found->second;if(request->value.active || request->value.completed || request->value.cancelled)return {};
    auto next=std::make_unique<Continuation>();next->state=state_;next->request=request;next->call=call;request->value.active=true;return next;
}
bool NativeTalkStringDrawing::UsesScheduler(const NativeProcessScheduler& scheduler) const noexcept{return state_->scheduler.lock().get()==&scheduler;}
bool NativeTalkStringDrawing::UsesComposition(const NativeTalkWindow& windows,const NativeFontDrawing& fonts) const noexcept{return state_->windows.get()==&windows && state_->fonts.get()==&fonts;}
void NativeTalkStringDrawing::ForgetDraw(TalkStringDrawRequest id) {
    const auto row=state_->Get(id);if(!row)return;
    if(row->value.child)state_->fonts->ForgetDraw(row->value.child);state_->requests.erase(id->serial);
}
}
