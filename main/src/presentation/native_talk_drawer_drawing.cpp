#include "fates/presentation/native_talk_drawer_drawing.hpp"
#include <algorithm>
#include <bit>
#include <cmath>
#include <limits>
#include <map>

namespace fates::presentation::native {
using namespace runtime::native;
using namespace io::native;
using S=TalkDrawerDrawStatus;
using E=TalkDrawerDrawEntry;
namespace {
constexpr std::uint32_t SetState=0x1f1120,Location=0x1f03f8,Absolute=0x1f0498,
    Face=0x1f0664,Stand=0x1f07b0,System=0x1f0934,NextIcon=0x1f058c,Characters=0x1f0c90,Variable=0x1f0db4;
constexpr std::array Targets{SetState,Location,Absolute,Face,Stand,System,NextIcon,Characters,Variable};
bool IsFrame(E entry){return entry>=E::FaceWindow && entry<=E::VariableStand;}
std::int32_t Signed16(std::uint32_t word){return std::bit_cast<std::int16_t>(static_cast<std::uint16_t>(word));}
std::uint32_t Bits(float value){return std::bit_cast<std::uint32_t>(value);}
std::uint32_t Target(E entry){return Targets[static_cast<std::size_t>(entry)];}
// Literal US SE v1.1 records at0x645018 (11x20 bytes), independently compared
// with executable bytes in the frame oracle. Unused row padding is not state.
struct Shout {bool active;std::array<std::uint32_t,4> bits;};
constexpr std::array<Shout,11> Shouts{{
    {false,{0,0,1065353216,1065353216}},
    {false,{0,1084227584,1064749236,1065353216}},
    {true,{0,1056964608,1065772646,1066611507}},
    {true,{0,3229614080u,1064682127,1065353216}},
    {true,{0,3204448256u,1065940419,1066192077}},
    {true,{0,1077936128,1065520988,1065772646}},
    {true,{0,3221225472u,1064849900,1065353216}},
    {true,{0,1073741824,1065470657,1065562931}},
    {true,{0,0,1065848144,1065772646}},
    {true,{0,1065353216,1065596486,1065562931}},
    {true,{0,1065353216,1065353216,1065353216}}
}};
constexpr std::array<std::uint32_t,9> NextOffsets{4,3,2,1,1,1,2,3,4}; //0x6450f4
bool Offset(std::uint32_t xy,std::uint32_t offset,std::uint32_t& out) {
    const float converted=static_cast<float>(std::bit_cast<std::int32_t>(xy));
    const float value=std::bit_cast<float>(offset)+converted;
    if(!std::isfinite(value) || value< -2147483648.0f || value>=2147483648.0f)return false;
    out=static_cast<std::uint32_t>(static_cast<std::int32_t>(value));return true;
}
ProcessCall Service(ProcessHandle process,std::uint32_t target,std::uint32_t arg) {
    ProcessCall call;call.process=std::move(process);call.kind=ProcessCallKind::Service;
    call.target=target;call.arguments[0]=arg;call.argument_count=1;return call;
}
}
struct NativeTalkDrawerDrawing::State {
    struct Request {
        TalkDrawerDrawObservation value;E entry{};std::uint32_t selector{};
        std::array<std::uint32_t,2> xy{};ColorReader color;
        TalkDrawerFrameOptions frame;TalkDrawerFrameInputs inputs;
        std::optional<std::array<FileBaseHandle,2>> captured_files;
    };
    std::weak_ptr<NativeProcessScheduler> scheduler;
    std::shared_ptr<NativeTalkWindowDrawer> drawer;std::shared_ptr<NativeFileBase> bases;
    std::shared_ptr<NativeTexFiles> tex;std::shared_ptr<NativeTextureObjects> textures;
    std::shared_ptr<NativeCoordinateResources> coordinates;std::shared_ptr<NativeTalkLayout> layout;
    std::shared_ptr<NativeCoordinateDrawing> positions;std::shared_ptr<PrimitiveDrawSink> sink;
    std::shared_ptr<NativeFontMetrics> metrics;
    std::map<std::uint32_t,std::shared_ptr<Request>> requests;std::uint32_t serial{};
    bool Live() const {const auto owner=scheduler.lock();return owner && owner->root(2);}
    S Mutable(ProcessAccess* access) const {
        const auto owner=scheduler.lock();if(!owner || !owner->root(2))return S::Retired;
        if(access)return access->BelongsTo(*owner)?S::Ready:S::MismatchedDomain;
        return owner->busy()?S::Busy:S::Ready;
    }
    std::shared_ptr<Request> Get(TalkDrawerDrawRequest id) const {
        if(!id)return {};const auto found=requests.find(id->serial);
        return found!=requests.end() && found->second->value.identity==id?found->second:nullptr;
    }
    void Forget(TalkDrawerDrawRequest id) {
        const auto row=Get(id);if(!row)return;
        if(row->value.state_child)Forget(row->value.state_child);
        if(row->value.frame_child)Forget(row->value.frame_child);
        if(row->value.coordinate_child)positions->ForgetDraw(row->value.coordinate_child);
        if(row->value.measurement && metrics)metrics->ForgetText(row->value.measurement);
        requests.erase(id->serial);
    }
    S Add(E entry,std::uint32_t selector,std::array<std::uint32_t,2> xy,ColorReader color,TalkDrawerDrawRequest& out) {
        if((entry!=E::State && entry!=E::LocationNamePlate && entry!=E::AbsoluteNamePlate) ||
           (entry==E::State?selector>1:!color))return S::InvalidRequest;
        if(serial==std::numeric_limits<std::uint32_t>::max())return S::IdentityExhausted;
        auto request=std::make_shared<Request>();request->value.identity=std::make_shared<TalkDrawerDrawIdentity>(++serial);
        request->entry=entry;request->selector=selector;request->xy=xy;request->color=std::move(color);
        requests.emplace(serial,request);out=request->value.identity;return S::Ready;
    }
    S AddFrame(E entry,TalkDrawerFrameOptions frame,TalkDrawerFrameInputs inputs,ColorReader color,TalkDrawerDrawRequest& out) {
        if(!IsFrame(entry) || !color)return S::InvalidRequest;
        if(serial==std::numeric_limits<std::uint32_t>::max())return S::IdentityExhausted;
        auto request=std::make_shared<Request>();request->value.identity=std::make_shared<TalkDrawerDrawIdentity>(++serial);
        request->entry=entry;request->frame=frame;request->inputs=std::move(inputs);request->color=std::move(color);
        requests.emplace(serial,request);out=request->value.identity;return S::Ready;
    }
    std::optional<ProcessCall> Call(ProcessHandle process,TalkDrawerDrawRequest id) const {
        const auto request=Get(id);
        if(!Live() || !request || request->value.active || request->value.completed || request->value.cancelled)return {};
        return Service(std::move(process),Target(request->entry),id->serial);
    }
    bool Valid(const CoordinateTextureBinding& binding) const {
        if(const auto texture=std::get_if<NativeTextureView>(&binding)) {
            NativeTextureDescription description;return textures->Describe(*texture,description)==TextureObjectStatus::Ready;
        }
        return true;
    }
};
struct NativeTalkDrawerDrawing::Continuation final:ProcessContinuation {
    enum class Stage {Start,FirstDone,WaitFirst,SecondDone,WaitSecond,Async,WaitAsync,Finish,WaitFinish,
        Begin,StateTexture,Texture,Shape,End,FrameState,WaitState,Capture,Find,Color,FrameTexture,Adjust,Position,WaitPosition,
        FrameCapture,ShoutSelect,StringFind,WindowFind,WindowTexture,WindowAdjust,Scaled,WaitScaled,
        IconSuppression,IconClock,IconFind,IconAdjust,IconColor,IconTexture,IconPosition,WaitIcon,
        VariableChild,WaitVariable,FontPush,WaitFontPush,VariablePush,FontSelect,WaitFontSelect,Measure,WaitMeasure,
        VariableFind,VariableBegin,VariableTexture,VariableBind,VariableEnd,TextureFind,VariableGeometry,TileGeometry,TileColor,TileEmit,TileAdvance,
        VariablePop,FontPop,WaitFontPop,Done};
    std::shared_ptr<State> state;std::shared_ptr<State::Request> request;ProcessCall call;
    Stage stage{Stage::Start};std::size_t index{};FileBaseHandle captured;
    UniqueArchiveRecord position;std::array<std::uint32_t,2> xy{};
    std::optional<CoordinateTextureBinding> binding;std::optional<PrimitiveCommand> pending;
    FileBaseHandle icon_file;const Shout* shout{};bool active_shout{};
    std::array<std::uint32_t,2> scale_bits{0x3f800000,0x3f800000};std::uint32_t icon_location{};
    std::array<UniqueArchiveRecord,3> tiles;std::size_t tile_index{};std::uint32_t tile_x{},tile_y{},stretch{};
    std::array<std::uint32_t,9> tile_geometry{};
    ~Continuation() override {
        if(!request)return;
        if(request->value.state_child){state->Forget(request->value.state_child);request->value.state_child.reset();}
        if(request->value.coordinate_child){state->positions->ForgetDraw(request->value.coordinate_child);request->value.coordinate_child.reset();}
        if(request->value.frame_child){state->Forget(request->value.frame_child);request->value.frame_child.reset();}
        if(request->value.measurement && state->metrics){state->metrics->ForgetText(request->value.measurement);request->value.measurement.reset();}
        if(!request->value.completed){request->value.active=false;request->value.cancelled=true;request->value.status=S::Cancelled;}
    }
    ProcessCallbackStep Block(S status=S::Unavailable){request->value.status=status;return ProcessCallbackStep::Blocked();}
    FileBaseHandle Current(std::size_t slot) const {
        const auto current=state->drawer->Observe();
        if(!current || !current->instance_present || !current->instance_live || slot>=current->files.size())return {};
        const auto file=current->files[slot];return state->bases->Observe(file)?file:nullptr;
    }
    ProcessCallbackStep Invoke(std::uint32_t target,Stage next) {
        // Original global loads happen at each call site, including both loops.
        const auto file=Current(index);if(!file)return Block();
        stage=next;return ProcessCallbackStep::Call(Service(call.process,target,file->serial));
    }
    bool Submit(PrimitiveCommand command) {
        if(!pending)pending=std::move(command);
        if(state->sink && !state->sink->Submit(*pending))return false;
        ++request->value.commands;pending.reset();return true;
    }
    bool Texture(FileBaseHandle file) {
        if(!state->bases->Observe(file))return false;
        const auto found=state->tex->GetTexture(file,0);
        if(found.status==TextureObjectStatus::Dummy){binding=PrimitiveDummyTexture{};return true;}
        if(found.status!=TextureObjectStatus::Ready)return false;
        binding=found.value;return true;
    }
    bool RecordValid() const {return position && state->coordinates->ReadU16(position,8).status==UniqueArchiveStatus::Ready;}
    ProcessCallbackStep Step(ProcessAccess& access) override {
        if(const auto status=state->Mutable(&access);status!=S::Ready)return Block(status);
        if(state->Get(request->value.identity)!=request)return Block(S::Cancelled);
        for(;;) {
            if(position && !RecordValid())return Block(S::InvalidRecord);
            if(binding && !state->Valid(*binding))return Block(S::InvalidTexture);
            for(const auto& tile:tiles)if(tile && state->coordinates->ReadU16(tile,8).status!=UniqueArchiveStatus::Ready)return Block(S::InvalidRecord);
            switch(stage) {
            case Stage::Start: {
                if(IsFrame(request->entry)){stage=request->entry==E::NextIcon || request->entry==E::VariableStand?Stage::FrameCapture:Stage::FrameState;break;}
                if(request->entry!=E::State){stage=request->entry==E::LocationNamePlate?Stage::FrameState:Stage::Capture;break;}
                const auto current=state->drawer->Observe();if(!current)return Block();
                if(!current->instance_present){stage=Stage::Done;break;}
                if(!current->instance_live)return Block();stage=Stage::FirstDone;break;
            }
            case Stage::FirstDone:
                if(index==2){stage=Stage::Begin;break;}return Invoke(0x545df4,Stage::WaitFirst);
            case Stage::WaitFirst:
                if(access.call_result()){++index;stage=Stage::FirstDone;}
                else {index=0;stage=Stage::SecondDone;}break;
            case Stage::SecondDone:
                if(index==2){stage=Stage::Begin;break;}return Invoke(0x545df4,Stage::WaitSecond);
            case Stage::WaitSecond:
                if(access.call_result()){++index;stage=Stage::SecondDone;}else stage=Stage::Async;break;
            case Stage::Async:return Invoke(0x545dd0,Stage::WaitAsync);
            case Stage::WaitAsync:
                if(access.call_result()){++index;stage=Stage::SecondDone;}else stage=Stage::Finish;break;
            case Stage::Finish: {
                const auto file=Current(index);if(!file)return Block();
                const auto child=state->bases->FinishCall(call.process,file);if(!child)return Block();
                stage=Stage::WaitFinish;return ProcessCallbackStep::Call(*child);
            }
            case Stage::WaitFinish:++index;stage=Stage::SecondDone;break;
            case Stage::Begin:if(!Submit(PrimitiveBegin{}))return Block();stage=Stage::StateTexture;break;
            case Stage::StateTexture:
                if(!Texture(Current(request->selector)))return Block(S::InvalidTexture);stage=Stage::Texture;break;
            case Stage::Texture:
                if(!Submit(std::visit([](const auto& value)->PrimitiveCommand{return PrimitiveTexture{value};},*binding)))return Block();
                stage=Stage::Shape;break;
            case Stage::Shape:if(!Submit(PrimitiveRectShape{0}))return Block();stage=Stage::End;break;
            case Stage::End:
                if(!Submit(PrimitiveEnd{}))return Block();request->value.returned=1;stage=Stage::Done;break;
            case Stage::FrameState: {
                if(!request->value.state_child) {
                    const auto status=state->Add(E::State,1,{},{},request->value.state_child);if(status!=S::Ready)return Block(status);
                }
                const auto child=state->Call(call.process,request->value.state_child);if(!child)return Block();
                stage=Stage::WaitState;return ProcessCallbackStep::Call(*child);
            }
            case Stage::WaitState: {
                const auto child=state->Get(request->value.state_child);if(!child || !child->value.completed)return Block();
                const bool proceed=child->value.returned!=0;state->requests.erase(request->value.state_child->serial);
                request->value.state_child.reset();stage=proceed?(IsFrame(request->entry)?Stage::FrameCapture:Stage::Capture):Stage::Done;break;
            }
            case Stage::Capture:
                captured=Current(0);if(!captured)return Block();stage=Stage::Find;break;
            case Stage::Find: {
                const auto found=state->layout->Find(TalkLayoutItem::NamePlate,request->entry==E::AbsoluteNamePlate?0:request->selector);
                if(found.status!=UniqueArchiveStatus::Ready || !found.value)return Block(S::InvalidRecord);
                position=found.value;stage=Stage::Color;break;
            }
            case Stage::Color:
                ++request->value.color_attempts;request->value.copied_color=request->color();
                if(!request->value.copied_color)return Block();stage=Stage::FrameTexture;break;
            case Stage::FrameTexture:
                if(!Texture(captured))return Block(S::InvalidTexture);stage=Stage::Adjust;break;
            case Stage::Adjust: {
                xy=request->xy;
                if(request->entry==E::AbsoluteNamePlate) {
                    const auto y=state->coordinates->ReadU16(position,10),x=state->coordinates->ReadU16(position,8);
                    if(y.status!=UniqueArchiveStatus::Ready || x.status!=UniqueArchiveStatus::Ready)return Block(S::InvalidRecord);
                    xy[0]-=static_cast<std::uint32_t>(static_cast<std::int32_t>(std::bit_cast<std::int16_t>(static_cast<std::uint16_t>(x.value))));
                    xy[1]-=static_cast<std::uint32_t>(static_cast<std::int32_t>(std::bit_cast<std::int16_t>(static_cast<std::uint16_t>(y.value))));
                }
                stage=Stage::Position;break;
            }
            case Stage::Position: {
                if(!request->value.coordinate_child) {
                    const auto color=*request->value.copied_color;
                    if(state->positions->Prepare(CoordinateDrawEntry::Position,position,binding,xy,[color]{return color;},true,
                       request->value.coordinate_child,&access)!=CoordinateDrawStatus::Ready)return Block();
                }
                const auto child=state->positions->DrawCall(call.process,request->value.coordinate_child);if(!child)return Block();
                stage=Stage::WaitPosition;return ProcessCallbackStep::Call(*child);
            }
            case Stage::WaitPosition: {
                const auto child=state->positions->Observe(request->value.coordinate_child);if(!child || !child->completed)return Block();
                if(state->positions->Release(request->value.coordinate_child,&access)!=CoordinateDrawStatus::Ready)return Block();
                request->value.coordinate_child.reset();stage=Stage::Done;break;
            }
            case Stage::FrameCapture: {
                // Capture the original instance's attachments once. Later icon
                // work must not silently switch to a replacement singleton.
                if(request->captured_files){icon_file=(*request->captured_files)[0];captured=(*request->captured_files)[1];}
                else {
                    const auto current=state->drawer->Observe();if(!current)return Block();
                    if(current->instance_present && current->instance_live){captured=current->files[1];icon_file=current->files[0];}
                    else if(request->entry!=E::NextIcon && request->entry!=E::VariableStand)return Block();
                }
                xy=request->frame.xy;icon_location=request->frame.location;
                if(request->entry==E::SystemWindow){xy={};icon_location=3;}
                stage=request->entry==E::NextIcon?Stage::IconSuppression:request->entry==E::VariableStand?Stage::FontPush:Stage::ShoutSelect;break;
            }
            case Stage::ShoutSelect:
                if((request->entry==E::StandWindow && request->frame.location==103) || request->entry==E::StandCharacters) {
                    if(!state->metrics)return Block(S::UnimplementedBranch);
                    stage=Stage::VariableChild;break;
                }
                if(request->entry!=E::SystemWindow && request->frame.shout_index>=0) {
                    const auto index_value=static_cast<std::size_t>(request->frame.shout_index);
                    if(index_value>=Shouts.size())return Block(S::InvalidGeometry);
                    shout=&Shouts[index_value];active_shout=shout->active;
                }
                stage=request->entry==E::FaceWindow?Stage::StringFind:Stage::WindowFind;break;
            case Stage::StringFind: {
                // Original Face gets this position even though its return is
                // unused. Preserve a reached lookup failure/barrier.
                const auto found=state->layout->Find(TalkLayoutItem::String,request->frame.location);
                if(found.status!=UniqueArchiveStatus::Ready && found.status!=UniqueArchiveStatus::Missing)return Block(S::InvalidRecord);
                stage=Stage::WindowFind;break;
            }
            case Stage::WindowFind: {
                const auto found=request->entry==E::FaceWindow?state->layout->FaceWindow(request->frame.location,active_shout):state->layout->StandWindow(active_shout);
                if(found.status==UniqueArchiveStatus::Missing || (found.status==UniqueArchiveStatus::Ready && !found.value)){stage=Stage::Done;break;}
                if(found.status!=UniqueArchiveStatus::Ready)return Block(S::InvalidRecord);
                position=found.value;stage=Stage::WindowTexture;break;
            }
            case Stage::WindowTexture:
                if(!Texture(captured))return Block(S::InvalidTexture);stage=Stage::WindowAdjust;break;
            case Stage::WindowAdjust:
                if(active_shout) {
                    if(!Offset(request->frame.xy[0],shout->bits[0],xy[0]) || !Offset(request->frame.xy[1],shout->bits[1],xy[1]))return Block(S::InvalidGeometry);
                    scale_bits={shout->bits[2],shout->bits[3]};
                }
                stage=Stage::Scaled;break;
            case Stage::Scaled: {
                if(!request->value.coordinate_child && state->positions->PrepareScaled(position,binding,xy,scale_bits,request->color,
                    request->value.coordinate_child,&access)!=CoordinateDrawStatus::Ready)return Block();
                const auto child=state->positions->DrawCall(call.process,request->value.coordinate_child);if(!child)return Block();
                stage=Stage::WaitScaled;return ProcessCallbackStep::Call(*child);
            }
            case Stage::WaitScaled: {
                const auto child=state->positions->Observe(request->value.coordinate_child);if(!child || !child->completed)return Block();
                if(state->positions->Release(request->value.coordinate_child,&access)!=CoordinateDrawStatus::Ready)return Block();
                request->value.coordinate_child.reset();position={};binding.reset();
                xy=request->entry==E::SystemWindow?std::array<std::uint32_t,2>{}:request->frame.xy;
                stage=request->frame.next_icon?Stage::IconSuppression:Stage::Done;break;
            }
            case Stage::IconSuppression: {
                if(!request->inputs.next_icon_suppressed)return Block();
                const auto word=request->inputs.next_icon_suppressed();if(!word)return Block();
                stage=*word?(request->entry==E::VariableStand?Stage::VariablePop:Stage::Done):Stage::IconClock;break;
            }
            case Stage::IconClock: {
                if(!request->inputs.elapsed_ticks)return Block();
                const auto word=request->inputs.elapsed_ticks();if(!word)return Block();
                xy[1]+=NextOffsets[(*word%36)/4];stage=Stage::IconFind;break;
            }
            case Stage::IconFind: {
                const auto found=state->layout->Find(TalkLayoutItem::NextIcon,icon_location);
                if(found.status!=UniqueArchiveStatus::Ready || !found.value)return Block(S::InvalidRecord);
                position=found.value;stage=Stage::IconAdjust;break;
            }
            case Stage::IconAdjust:
                if(icon_location==103) {
                    const auto x=state->coordinates->ReadU16(position,8);if(x.status!=UniqueArchiveStatus::Ready)return Block(S::InvalidRecord);
                    xy[0]-=static_cast<std::uint32_t>(static_cast<std::int32_t>(std::bit_cast<std::int16_t>(static_cast<std::uint16_t>(x.value))));
                }
                stage=Stage::IconColor;break;
            case Stage::IconColor:
                ++request->value.color_attempts;request->value.copied_color=request->color();
                if(!request->value.copied_color)return Block();stage=Stage::IconTexture;break;
            case Stage::IconTexture:
                if(!Texture(icon_file))return Block(S::InvalidTexture);stage=Stage::IconPosition;break;
            case Stage::IconPosition: {
                if(!request->value.coordinate_child) {
                    const auto color=*request->value.copied_color;
                    if(state->positions->Prepare(CoordinateDrawEntry::Position,position,binding,xy,[color]{return color;},true,
                        request->value.coordinate_child,&access)!=CoordinateDrawStatus::Ready)return Block();
                }
                const auto child=state->positions->DrawCall(call.process,request->value.coordinate_child);if(!child)return Block();
                stage=Stage::WaitIcon;return ProcessCallbackStep::Call(*child);
            }
            case Stage::WaitIcon: {
                const auto child=state->positions->Observe(request->value.coordinate_child);if(!child || !child->completed)return Block();
                if(state->positions->Release(request->value.coordinate_child,&access)!=CoordinateDrawStatus::Ready)return Block();
                request->value.coordinate_child.reset();stage=request->entry==E::VariableStand?Stage::VariablePop:Stage::Done;break;
            }
            case Stage::VariableChild: {
                if(!request->value.frame_child) {
                    auto options=request->frame;if(request->entry==E::StandWindow)options.character_count=11;else options.next_icon=false;
                    const auto status=state->AddFrame(E::VariableStand,options,request->inputs,request->color,request->value.frame_child);if(status!=S::Ready)return Block(status);
                    state->Get(request->value.frame_child)->captured_files=std::array{icon_file,captured};
                }
                const auto child=state->Call(call.process,request->value.frame_child);if(!child)return Block();
                stage=Stage::WaitVariable;return ProcessCallbackStep::Call(*child);
            }
            case Stage::WaitVariable: {
                const auto child=state->Get(request->value.frame_child);if(!child || !child->value.completed)return Block();
                request->value.measured_width=child->value.measured_width;state->requests.erase(request->value.frame_child->serial);
                request->value.frame_child.reset();stage=Stage::Done;break;
            }
            case Stage::FontPush:
                if(!state->metrics)return Block();stage=Stage::WaitFontPush;return ProcessCallbackStep::Call(NativeFontMetrics::PushCall(call.process));
            case Stage::WaitFontPush:stage=Stage::VariablePush;break;
            case Stage::VariablePush:if(!Submit(PrimitivePush{}))return Block();stage=Stage::FontSelect;break;
            case Stage::FontSelect:stage=Stage::WaitFontSelect;return ProcessCallbackStep::Call(NativeFontMetrics::SetCurrentCall(call.process,1));
            case Stage::WaitFontSelect:stage=Stage::Measure;break;
            case Stage::Measure: {
                if(!request->value.measurement && state->metrics->PrepareText([](std::size_t at)->std::optional<char16_t>{return at==0?u'0':u'\0';},
                    FontTextOperation::Width,request->value.measurement,&access)!=FontMetricStatus::Ready)return Block();
                const auto child=state->metrics->TextCall(call.process,request->value.measurement);if(!child)return Block();
                stage=Stage::WaitMeasure;return ProcessCallbackStep::Call(*child);
            }
            case Stage::WaitMeasure: {
                const auto child=state->metrics->Observe(request->value.measurement);if(!child || !child->completed || !child->result)return Block();
                request->value.measured_width=child->result;
                if(state->metrics->Release(request->value.measurement,&access)!=FontMetricStatus::Ready)return Block();
                request->value.measurement.reset();stretch=*child->result*request->frame.character_count-92u;stage=Stage::VariableFind;break;
            }
            case Stage::VariableFind: {
                const auto found=state->coordinates->Find("ShopBuyD_TalkW_VariableTalkW1");
                if(found.status==UniqueArchiveStatus::Missing || (found.status==UniqueArchiveStatus::Ready && !found.value)){stage=Stage::VariablePop;break;}
                if(found.status!=UniqueArchiveStatus::Ready)return Block(S::InvalidRecord);
                position=found.value;stage=Stage::VariableBegin;break;
            }
            case Stage::VariableBegin:if(!Submit(PrimitiveBegin{}))return Block();stage=Stage::VariableTexture;break;
            case Stage::VariableTexture:if(!Texture(captured))return Block(S::InvalidTexture);stage=Stage::VariableBind;break;
            case Stage::VariableBind:
                if(!Submit(std::visit([](const auto& value)->PrimitiveCommand{return PrimitiveTexture{value};},*binding)))return Block();stage=Stage::VariableEnd;break;
            case Stage::VariableEnd:if(!Submit(PrimitiveEnd{}))return Block();stage=Stage::TextureFind;break;
            case Stage::TextureFind: {
                constexpr std::array names{"TalkW_VariableTalkW1","TalkW_VariableTalkW2","TalkW_VariableTalkW3"};
                const auto found=state->coordinates->Find(names[tile_index]);if(found.status!=UniqueArchiveStatus::Ready || !found.value)return Block(S::InvalidRecord);
                tiles[tile_index]=found.value;if(++tile_index==tiles.size()){tile_index=0;stage=Stage::VariableGeometry;}break;
            }
            case Stage::VariableGeometry: {
                const auto x=state->coordinates->ReadU16(position,8),y=state->coordinates->ReadU16(position,10);
                if(x.status!=UniqueArchiveStatus::Ready || y.status!=UniqueArchiveStatus::Ready)return Block(S::InvalidRecord);
                tile_x=request->frame.xy[0]+static_cast<std::uint32_t>(Signed16(x.value));tile_y=request->frame.xy[1]+static_cast<std::uint32_t>(Signed16(y.value));
                stage=Stage::TileGeometry;break;
            }
            case Stage::TileGeometry: {
                std::array<std::int32_t,4> values{};
                for(std::size_t i=0;i<values.size();++i){const auto word=state->coordinates->ReadU16(tiles[tile_index],8+2*i);
                    if(word.status!=UniqueArchiveStatus::Ready)return Block(S::InvalidRecord);values[i]=Signed16(word.value);}
                const auto u=values[0],v=values[1],w=values[2],h=values[3];
                const auto width=tile_index==1?std::bit_cast<std::int32_t>(stretch):Signed16(static_cast<std::uint32_t>(w<0?-w:w));
                tile_geometry={Bits(static_cast<float>(std::bit_cast<std::int32_t>(tile_x))),Bits(static_cast<float>(std::bit_cast<std::int32_t>(tile_y))),0,
                    Bits(static_cast<float>(width)),Bits(static_cast<float>(Signed16(static_cast<std::uint32_t>(h<0?-h:h)))),Bits(static_cast<float>(u)),Bits(static_cast<float>(v)),
                    Bits(static_cast<float>(Signed16(static_cast<std::uint32_t>(u+w)))),Bits(static_cast<float>(Signed16(static_cast<std::uint32_t>(v+h))))};
                stage=Stage::TileColor;break;
            }
            case Stage::TileColor:
                if(!request->inputs.variable_color)return Block();++request->value.color_attempts;request->value.copied_color=request->inputs.variable_color();
                if(!request->value.copied_color)return Block();stage=Stage::TileEmit;break;
            case Stage::TileEmit:
                if(!Submit(PrimitiveUvRectangle{tile_geometry,*request->value.copied_color}))return Block();stage=Stage::TileAdvance;break;
            case Stage::TileAdvance:
                if(tile_index==0) {
                    const auto word=state->coordinates->ReadU16(tiles[0],12);if(word.status!=UniqueArchiveStatus::Ready)return Block(S::InvalidRecord);
                    const auto width=Signed16(word.value);tile_x+=static_cast<std::uint32_t>(width<0?-width:width);
                } else if(tile_index==1)tile_x+=stretch;
                if(++tile_index<tiles.size()){stage=Stage::TileGeometry;break;}
                position={};tiles={};binding.reset();xy={tile_x+68u,request->frame.xy[1]};
                stage=request->frame.next_icon?Stage::IconSuppression:Stage::VariablePop;break;
            case Stage::VariablePop:if(!Submit(PrimitivePop{}))return Block();stage=Stage::FontPop;break;
            case Stage::FontPop:stage=Stage::WaitFontPop;return ProcessCallbackStep::Call(NativeFontMetrics::PopCall(call.process));
            case Stage::WaitFontPop:stage=Stage::Done;break;
            case Stage::Done:
                request->value.active=false;request->value.completed=true;request->value.status=S::Ready;
                return ProcessCallbackStep::Return(request->value.returned);
            }
        }
    }
};
NativeTalkDrawerDrawing::NativeTalkDrawerDrawing(std::shared_ptr<State> state):state_(std::move(state)){}
NativeTalkDrawerDrawing::~NativeTalkDrawerDrawing()=default;
S NativeTalkDrawerDrawing::Create(std::shared_ptr<NativeProcessScheduler> scheduler,std::shared_ptr<ProcessCallbackRegistry> registry,
    std::shared_ptr<NativeTalkWindowDrawer> drawer,std::shared_ptr<NativeFileController> files,std::shared_ptr<NativeFileBase> bases,
    std::shared_ptr<NativeTexFiles> tex,std::shared_ptr<NativeTextureObjects> textures,std::shared_ptr<NativeCoordinateResources> coordinates,
    std::shared_ptr<NativeTalkLayout> layout,std::shared_ptr<NativeCoordinateDrawing> positions,std::shared_ptr<PrimitiveDrawSink> sink,
    std::shared_ptr<NativeTalkDrawerDrawing>& out,std::shared_ptr<NativeFontMetrics> metrics) {
    if(!scheduler || !scheduler->root(2))return S::NullScheduler;
    if(!registry || !scheduler->UsesCallbacks(registry.get()) || !drawer || !files || !bases || !tex || !textures || !coordinates || !layout || !positions ||
       !drawer->UsesScheduler(*scheduler) || !drawer->UsesOwners(*files,*bases,*tex,*coordinates) || !tex->UsesTextureObjects(*textures) ||
       !positions->UsesOwners(*coordinates,*textures) || !positions->UsesSink(sink.get()) || !layout->UsesScheduler(*scheduler) ||
       (metrics && (!metrics->UsesScheduler(*scheduler) || !metrics->UsesFileBaseOwners(*files,*bases))))return S::MismatchedDomain;
    auto state=std::make_shared<State>();state->scheduler=scheduler;state->drawer=std::move(drawer);state->bases=std::move(bases);
    state->tex=std::move(tex);state->textures=std::move(textures);state->coordinates=std::move(coordinates);state->layout=std::move(layout);
    state->positions=std::move(positions);state->sink=std::move(sink);
    state->metrics=std::move(metrics);
    auto next=std::shared_ptr<NativeTalkDrawerDrawing>(new NativeTalkDrawerDrawing(state));
    if(!registry->Register(Targets,next))return S::DuplicateBinding;
    out=std::move(next);return S::Ready;
}
S NativeTalkDrawerDrawing::Prepare(E entry,std::uint32_t selector,std::array<std::uint32_t,2> xy,ColorReader color,
    TalkDrawerDrawRequest& out,ProcessAccess* access) {
    if(const auto status=state_->Mutable(access);status!=S::Ready)return status;
    return state_->Add(entry,selector,xy,std::move(color),out);
}
S NativeTalkDrawerDrawing::PrepareFrame(E entry,TalkDrawerFrameOptions frame,TalkDrawerFrameInputs inputs,ColorReader color,
    TalkDrawerDrawRequest& out,ProcessAccess* access) {
    if(const auto status=state_->Mutable(access);status!=S::Ready)return status;
    return state_->AddFrame(entry,frame,std::move(inputs),std::move(color),out);
}
std::optional<ProcessCall> NativeTalkDrawerDrawing::DrawCall(ProcessHandle process,TalkDrawerDrawRequest request) const {return state_->Call(std::move(process),std::move(request));}
std::optional<TalkDrawerDrawObservation> NativeTalkDrawerDrawing::Observe(TalkDrawerDrawRequest request) const {
    const auto row=state_->Get(request);return state_->Live() && row?std::optional{row->value}:std::nullopt;
}
S NativeTalkDrawerDrawing::Release(TalkDrawerDrawRequest request,ProcessAccess* access) {
    if(const auto status=state_->Mutable(access);status!=S::Ready)return status;
    const auto row=state_->Get(request);if(!row)return S::InvalidRequest;if(row->value.active)return S::Busy;
    state_->requests.erase(request->serial);return S::Ready;
}
bool NativeTalkDrawerDrawing::UsesScheduler(const NativeProcessScheduler& scheduler) const noexcept{return state_->scheduler.lock().get()==&scheduler;}
bool NativeTalkDrawerDrawing::UsesMetrics(const NativeFontMetrics& metrics) const noexcept{return state_->metrics.get()==&metrics;}
bool NativeTalkDrawerDrawing::UsesComposition(const NativeCoordinateResources& coordinates,const NativeTalkLayout& layout,const PrimitiveDrawSink* sink) const noexcept {
    return state_->coordinates.get()==&coordinates && state_->layout.get()==&layout && state_->sink.get()==sink;
}
void NativeTalkDrawerDrawing::ForgetDraw(TalkDrawerDrawRequest request) {
    state_->Forget(request);
}
std::unique_ptr<ProcessContinuation> NativeTalkDrawerDrawing::Begin(const ProcessCall& call) {
    if(!state_->Live() || !call.process || call.kind!=ProcessCallKind::Service || call.has_self || call.this_adjustment || call.argument_count!=1 ||
       std::find(Targets.begin(),Targets.end(),call.target)==Targets.end())return {};
    const auto found=state_->requests.find(call.arguments[0]);if(found==state_->requests.end())return {};
    const auto request=found->second;
    if(call.target!=Target(request->entry) || request->value.active || request->value.completed || request->value.cancelled)return {};
    auto next=std::make_unique<Continuation>();next->state=state_;next->request=request;next->call=call;
    request->value.active=true;return next;
}
}
