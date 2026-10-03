#include "fates/presentation/native_coordinate_drawing.hpp"
#include <bit>
#include <cmath>
#include <limits>
#include <map>

namespace fates::presentation::native {
using namespace runtime::native;
using namespace io::native;
using S=CoordinateDrawStatus;
namespace {
constexpr std::uint32_t Draw=0x50cd80,Rect=0x50ce08,Scaled=0x1684a0;
std::uint32_t Target(CoordinateDrawEntry entry){return entry==CoordinateDrawEntry::Position?Draw:entry==CoordinateDrawEntry::Rectangle?Rect:Scaled;}
std::int32_t Signed16(std::uint32_t value){return std::bit_cast<std::int16_t>(static_cast<std::uint16_t>(value));}
std::uint32_t Bits(std::int32_t value){return std::bit_cast<std::uint32_t>(static_cast<float>(value));}
std::optional<std::int32_t> Narrow(float value) {
    if(!std::isfinite(value) || value < -2147483648.f || value >= 2147483648.f)return {};
    return Signed16(static_cast<std::uint32_t>(static_cast<std::int32_t>(value)));
}
}
struct NativeCoordinateDrawing::State {
    struct Request {
        CoordinateDrawObservation value;CoordinateDrawEntry entry{};UniqueArchiveRecord position;
        std::optional<CoordinateTextureBinding> texture;std::array<std::uint32_t,2> xy{};ColorReader color;bool depth{},guard_binding{};
        std::array<std::uint32_t,2> scale{0x3f800000,0x3f800000};
    };
    std::weak_ptr<NativeProcessScheduler> scheduler;std::shared_ptr<NativeCoordinateResources> coordinates;
    std::shared_ptr<NativeTextureObjects> textures;std::shared_ptr<PrimitiveDrawSink> sink;
    std::map<std::uint32_t,std::shared_ptr<Request>> requests;std::uint32_t serial{};
    bool Live() const {const auto s=scheduler.lock();return s && s->root(2);}
    S Mutable(ProcessAccess* access) const {
        const auto s=scheduler.lock();if(!s || !s->root(2))return S::Retired;
        if(access)return access->BelongsTo(*s)?S::Ready:S::MismatchedDomain;
        return s->busy()?S::Busy:S::Ready;
    }
    bool Valid(const UniqueArchiveRecord& record) const {return record && coordinates->ReadU16(record,8).status==UniqueArchiveStatus::Ready;}
    bool Valid(const std::optional<CoordinateTextureBinding>& binding) const {
        if(!binding)return false;
        if(const auto view=std::get_if<NativeTextureView>(&*binding)) {
            NativeTextureDescription description;return textures->Describe(*view,description)==TextureObjectStatus::Ready;
        }
        return true;
    }
    std::shared_ptr<Request> Get(CoordinateDrawRequest identity) const {
        if(!identity)return {};
        const auto found=requests.find(identity->serial);
        return found!=requests.end() && found->second->value.identity==identity?found->second:nullptr;
    }
    S Add(CoordinateDrawEntry entry,UniqueArchiveRecord position,std::optional<CoordinateTextureBinding> texture,
        std::array<std::uint32_t,2> xy,ColorReader color,bool depth,CoordinateDrawRequest& out,
        std::array<std::uint32_t,2> scale={0x3f800000,0x3f800000}) {
        if(entry!=CoordinateDrawEntry::Position && entry!=CoordinateDrawEntry::Rectangle && entry!=CoordinateDrawEntry::Scaled)return S::InvalidRequest;
        if(!Valid(position))return S::InvalidRecord;
        if(!color)return S::InvalidRequest;
        if(serial==std::numeric_limits<std::uint32_t>::max())return S::IdentityExhausted;
        auto request=std::make_shared<Request>();request->value.identity=std::make_shared<CoordinateDrawIdentity>(++serial);
        request->entry=entry;request->position=std::move(position);request->texture=std::move(texture);request->xy=xy;
        request->color=std::move(color);request->depth=depth;request->scale=scale;requests.emplace(serial,request);out=request->value.identity;return S::Ready;
    }
    std::optional<ProcessCall> Call(ProcessHandle process,CoordinateDrawRequest identity) const {
        const auto request=Get(identity);
        if(!Live() || !request || request->value.active || request->value.completed || request->value.cancelled)return {};
        ProcessCall call;call.process=std::move(process);call.kind=ProcessCallKind::Service;
        call.target=Target(request->entry);call.arguments[0]=identity->serial;call.argument_count=1;return call;
    }
};
struct NativeCoordinateDrawing::Continuation final:ProcessContinuation {
    enum class Stage {Push,SecondPush,Begin,Texture,Depth,End,CopyColor,Child,WaitChild,Pop,SecondPop,Geometry,RectColor,Rectangle,Done};
    std::shared_ptr<State> state;std::shared_ptr<State::Request> request;ProcessCall call;
    Stage stage{Stage::Push};std::optional<PrimitiveCommand> pending;bool bound{};UniqueArchiveRecord descriptor;
    ~Continuation() override {
        if(!request)return;
        if(request->value.child){state->requests.erase(request->value.child->serial);request->value.child.reset();}
        if(!request->value.completed){request->value.active=false;request->value.cancelled=true;request->value.status=S::Cancelled;}
    }
    ProcessCallbackStep Block(S status=S::Unavailable){request->value.status=status;return ProcessCallbackStep::Blocked();}
    bool Submit(PrimitiveCommand command) {
        if(!pending)pending=std::move(command);
        if(state->sink && !state->sink->Submit(*pending))return false;
        ++request->value.commands;pending.reset();return true;
    }
    std::optional<std::int32_t> Read(const UniqueArchiveRecord& record,std::size_t field) const {
        const auto value=state->coordinates->ReadU16(record,field);
        return value.status==UniqueArchiveStatus::Ready?std::optional{Signed16(value.value)}:std::nullopt;
    }
    bool Geometry() {
        const auto px=Read(request->position,8),py=Read(request->position,10),w=Read(request->position,12),h=Read(request->position,14);
        const auto texture=state->coordinates->Pointer(request->position,4);
        if(!px || !py || !w || !h || texture.status!=UniqueArchiveStatus::Ready)return false;
        descriptor=texture.value;
        const auto x=Bits(std::bit_cast<std::int32_t>(request->xy[0]+static_cast<std::uint32_t>(*px)));
        const auto y=Bits(std::bit_cast<std::int32_t>(request->xy[1]+static_cast<std::uint32_t>(*py)));
        if(!descriptor){pending=PrimitiveFlatRectangle{{x,y,0,Bits(*w),Bits(*h)},{}};return true;}
        const auto u=Read(descriptor,8),v=Read(descriptor,10),tw=Read(descriptor,12),th=Read(descriptor,14);
        if(!u || !v || !tw || !th)return false;
        const auto width=*w>0?*w:Signed16(static_cast<std::uint32_t>(*tw<0?-*tw:*tw));
        const auto height=*h>0?*h:Signed16(static_cast<std::uint32_t>(*th<0?-*th:*th));
        const auto ue=Signed16(static_cast<std::uint32_t>(*u)+static_cast<std::uint16_t>(*tw));
        const auto ve=Signed16(static_cast<std::uint32_t>(*v)+static_cast<std::uint16_t>(*th));
        pending=PrimitiveUvRectangle{{x,y,0,Bits(width),Bits(height),Bits(*u),Bits(*v),Bits(ue),Bits(ve)},{}};return true;
    }
    S ScaledGeometry() {
        const auto texture=state->coordinates->Pointer(request->position,4);
        if(texture.status!=UniqueArchiveStatus::Ready || !texture.value)return S::InvalidRecord;
        descriptor=texture.value;
        const auto u=Read(descriptor,8),v=Read(descriptor,10),tw=Read(descriptor,12),th=Read(descriptor,14);
        const auto px=Read(request->position,8),py=Read(request->position,10),pw=Read(request->position,12),ph=Read(request->position,14);
        if(!u || !v || !tw || !th || !px || !py || !pw || !ph)return S::InvalidRecord;
        const auto width=*pw>0?*pw:Signed16(static_cast<std::uint32_t>(*tw<0?-*tw:*tw));
        const auto height=*ph>0?*ph:Signed16(static_cast<std::uint32_t>(*th<0?-*th:*th));
        const float sx=std::bit_cast<float>(request->scale[0]),sy=std::bit_cast<float>(request->scale[1]);
        const float w=static_cast<float>(width)*sx,h=static_cast<float>(height)*sy;
        // ARM VMLA/VMLS are not fused. Keep each multiply and following add or
        // subtract separately rounded, including the centered scale correction.
        const float dx=static_cast<float>(std::bit_cast<std::int32_t>(request->xy[0]))*sx;
        const float dy=static_cast<float>(std::bit_cast<std::int32_t>(request->xy[1]))*sy;
        const float x=static_cast<float>(*px)+dx,y=static_cast<float>(*py)+dy;
        const float extra_x=w-static_cast<float>(width),extra_y=h-static_cast<float>(height);
        const float half_x=extra_x*.5f,half_y=extra_y*.5f;
        const auto ix=Narrow(x-half_x),iy=Narrow(y-half_y),iw=Narrow(w),ih=Narrow(h);
        if(!ix || !iy || !iw || !ih)return S::InvalidGeometry;
        const auto ue=Signed16(static_cast<std::uint32_t>(*u)+static_cast<std::uint32_t>(*tw<0?-*tw:*tw));
        const auto ve=Signed16(static_cast<std::uint32_t>(*v)+static_cast<std::uint32_t>(*th<0?-*th:*th));
        pending=PrimitiveUvRectangle{{Bits(*ix),Bits(*iy),0,Bits(*iw),Bits(*ih),Bits(*u),Bits(*v),Bits(ue),Bits(ve)},{}};return S::Ready;
    }
    ProcessCallbackStep Step(ProcessAccess& access) override {
        if(const auto status=state->Mutable(&access);status!=S::Ready)return Block(status);
        if(state->Get(request->value.identity)!=request)return Block(S::Cancelled);
        for(;;) {
            if(!state->Valid(request->position))return Block(S::InvalidRecord);
            if(bound && !state->Valid(request->texture))return Block(S::InvalidTexture);
            switch(stage) {
            case Stage::Push:if(!Submit(PrimitivePush{}))return Block();stage=request->entry==CoordinateDrawEntry::Scaled?Stage::SecondPush:Stage::Begin;break;
            case Stage::SecondPush:if(!Submit(PrimitivePush{}))return Block();stage=Stage::Begin;break;
            case Stage::Begin:if(!Submit(PrimitiveBegin{}))return Block();stage=Stage::Texture;break;
            case Stage::Texture:
                if(!state->Valid(request->texture))return Block(S::InvalidTexture);
                if(!Submit(std::visit([](const auto& value)->PrimitiveCommand{return PrimitiveTexture{value};},*request->texture)))return Block();
                bound=true;stage=request->depth?Stage::Depth:Stage::End;break;
            case Stage::Depth: {
                const auto value=Read(request->position,16);if(!value)return Block(S::InvalidRecord);
                if(!Submit(PrimitiveDepth{Bits(*value)}))return Block();stage=Stage::End;break;
            }
            case Stage::End:if(!Submit(PrimitiveEnd{}))return Block();stage=request->entry==CoordinateDrawEntry::Scaled?Stage::Geometry:Stage::CopyColor;break;
            case Stage::CopyColor:
                ++request->value.color_attempts;request->value.copied_color=request->color();
                if(!request->value.copied_color)return Block();stage=Stage::Child;break;
            case Stage::Child: {
                const auto color=*request->value.copied_color;
                const auto status=state->Add(CoordinateDrawEntry::Rectangle,request->position,request->texture,request->xy,[color]{return color;},false,request->value.child);
                if(status!=S::Ready)return Block(status);
                state->Get(request->value.child)->guard_binding=true;
                const auto child=state->Call(call.process,request->value.child);if(!child)return Block();
                stage=Stage::WaitChild;return ProcessCallbackStep::Call(*child);
            }
            case Stage::WaitChild: {
                const auto child=state->Get(request->value.child);if(!child || !child->value.completed)return Block();
                state->requests.erase(request->value.child->serial);request->value.child.reset();stage=Stage::Pop;break;
            }
            case Stage::Pop:if(!Submit(PrimitivePop{}))return Block();stage=request->entry==CoordinateDrawEntry::Scaled?Stage::SecondPop:Stage::Done;break;
            case Stage::SecondPop:if(!Submit(PrimitivePop{}))return Block();stage=Stage::Done;break;
            case Stage::Geometry:
                if(request->entry==CoordinateDrawEntry::Scaled){const auto status=ScaledGeometry();if(status!=S::Ready)return Block(status);}
                else if(!Geometry())return Block(S::InvalidRecord);
                stage=Stage::RectColor;break;
            case Stage::RectColor: {
                ++request->value.color_attempts;const auto color=request->color();if(!color)return Block();
                request->value.copied_color=*color;
                if(auto uv=std::get_if<PrimitiveUvRectangle>(&*pending))uv->color=*color;
                else std::get<PrimitiveFlatRectangle>(*pending).color=*color;
                stage=Stage::Rectangle;break;
            }
            case Stage::Rectangle:
                if(descriptor && !state->Valid(descriptor))return Block(S::InvalidRecord);
                if(!Submit(*pending))return Block();stage=request->entry==CoordinateDrawEntry::Scaled?Stage::Pop:Stage::Done;break;
            case Stage::Done:
                request->value.active=false;request->value.completed=true;request->value.status=S::Ready;return ProcessCallbackStep::Return();
            }
        }
    }
};
NativeCoordinateDrawing::NativeCoordinateDrawing(std::shared_ptr<State> state):state_(std::move(state)){}
NativeCoordinateDrawing::~NativeCoordinateDrawing()=default;
S NativeCoordinateDrawing::Create(std::shared_ptr<NativeProcessScheduler> scheduler,std::shared_ptr<ProcessCallbackRegistry> registry,
    std::shared_ptr<NativeCoordinateResources> coordinates,std::shared_ptr<NativeTextureObjects> textures,
    std::shared_ptr<PrimitiveDrawSink> sink,std::shared_ptr<NativeCoordinateDrawing>& out) {
    if(!scheduler || !scheduler->root(2))return S::NullScheduler;
    if(!registry || !scheduler->UsesCallbacks(registry.get()) || !coordinates || !textures ||
       !coordinates->UsesScheduler(*scheduler) || !textures->UsesScheduler(*scheduler))return S::MismatchedDomain;
    auto state=std::make_shared<State>();state->scheduler=scheduler;state->coordinates=std::move(coordinates);
    state->textures=std::move(textures);state->sink=std::move(sink);
    auto next=std::shared_ptr<NativeCoordinateDrawing>(new NativeCoordinateDrawing(state));
    const std::array targets{Draw,Rect,Scaled};if(!registry->Register(targets,next))return S::DuplicateBinding;
    out=std::move(next);return S::Ready;
}
S NativeCoordinateDrawing::Prepare(CoordinateDrawEntry entry,UniqueArchiveRecord position,std::optional<CoordinateTextureBinding> texture,
    std::array<std::uint32_t,2> xy,ColorReader color,bool depth,CoordinateDrawRequest& out,ProcessAccess* access) {
    if(const auto status=state_->Mutable(access);status!=S::Ready)return status;
    if(entry==CoordinateDrawEntry::Scaled)return S::InvalidRequest; // Requires explicit scale input.
    return state_->Add(entry,std::move(position),std::move(texture),xy,std::move(color),depth,out);
}
S NativeCoordinateDrawing::PrepareScaled(UniqueArchiveRecord position,std::optional<CoordinateTextureBinding> texture,
    std::array<std::uint32_t,2> xy,std::array<std::uint32_t,2> scale,ColorReader color,CoordinateDrawRequest& out,ProcessAccess* access) {
    if(const auto status=state_->Mutable(access);status!=S::Ready)return status;
    return state_->Add(CoordinateDrawEntry::Scaled,std::move(position),std::move(texture),xy,std::move(color),true,out,scale);
}
std::optional<ProcessCall> NativeCoordinateDrawing::DrawCall(ProcessHandle process,CoordinateDrawRequest identity) const {return state_->Call(std::move(process),std::move(identity));}
std::optional<CoordinateDrawObservation> NativeCoordinateDrawing::Observe(CoordinateDrawRequest identity) const {
    const auto request=state_->Get(identity);return state_->Live() && request?std::optional{request->value}:std::nullopt;
}
S NativeCoordinateDrawing::Release(CoordinateDrawRequest identity,ProcessAccess* access) {
    if(const auto status=state_->Mutable(access);status!=S::Ready)return status;
    const auto request=state_->Get(identity);if(!request)return S::InvalidRequest;if(request->value.active)return S::Busy;
    state_->requests.erase(identity->serial);return S::Ready;
}
bool NativeCoordinateDrawing::UsesScheduler(const NativeProcessScheduler& scheduler) const noexcept{return state_->scheduler.lock().get()==&scheduler;}
bool NativeCoordinateDrawing::UsesOwners(const NativeCoordinateResources& coordinates,const NativeTextureObjects& textures) const noexcept {
    return state_->coordinates.get()==&coordinates && state_->textures.get()==&textures;
}
bool NativeCoordinateDrawing::UsesSink(const PrimitiveDrawSink* sink) const noexcept{return state_->sink.get()==sink;}
void NativeCoordinateDrawing::ForgetDraw(CoordinateDrawRequest request) {
    const auto row=state_->Get(request);if(!row)return;
    if(row->value.child)ForgetDraw(row->value.child);
    state_->requests.erase(request->serial);
}
std::unique_ptr<ProcessContinuation> NativeCoordinateDrawing::Begin(const ProcessCall& call) {
    if(!state_->Live() || !call.process || call.kind!=ProcessCallKind::Service || call.has_self || call.this_adjustment ||
       (call.target!=Draw && call.target!=Rect && call.target!=Scaled) || call.argument_count!=1)return {};
    const auto found=state_->requests.find(call.arguments[0]);if(found==state_->requests.end())return {};
    const auto request=found->second;
    if(call.target!=Target(request->entry) || request->value.active || request->value.completed || request->value.cancelled)return {};
    auto next=std::make_unique<Continuation>();next->state=state_;next->request=request;next->call=call;
    next->bound=request->guard_binding;
    if(request->entry==CoordinateDrawEntry::Rectangle)next->stage=Continuation::Stage::Geometry;
    request->value.active=true;return next;
}
}
