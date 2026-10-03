#include "fates/presentation/native_talk_background.hpp"
#include <algorithm>
#include <bit>
#include <limits>
#include <map>

namespace fates::presentation::native {
using namespace runtime::native;
using namespace io::native;
namespace {
using S=TalkBackgroundStatus;
constexpr std::uint32_t Load=0x44c6f8,Draw=0x44c5f8,Destroy=0x44c830;
constexpr std::array Targets{Load,Draw,Destroy};
ProcessCall Service(ProcessHandle process,std::uint32_t target,std::initializer_list<std::uint32_t> args) {
    ProcessCall call;call.process=std::move(process);call.kind=ProcessCallKind::Service;call.target=target;
    call.argument_count=static_cast<std::uint8_t>(args.size());std::copy(args.begin(),args.end(),call.arguments.begin());return call;
}
}
struct NativeTalkBackground::State {
    std::weak_ptr<NativeProcessScheduler> scheduler;
    std::shared_ptr<ObjectHandleRegistry> objects;
    std::shared_ptr<NativeFileController> files;std::shared_ptr<NativeFileBase> bases;
    std::shared_ptr<NativeTexFiles> tex;std::shared_ptr<NativeTextureObjects> textures;
    std::shared_ptr<TalkBackgroundSink> sink;
    struct Name {TalkBackgroundName identity;std::string value;};
    std::map<std::uint32_t,TalkBackgroundState> rows;std::map<std::uint32_t,Name> names;
    std::uint32_t serial{},name_serial{};
    bool Live() const {auto s=scheduler.lock();return s && s->root(2);}
    S Mutable(ProcessAccess* access) const {
        auto s=scheduler.lock();if(!s || !s->root(2))return S::Retired;
        if(access)return access->BelongsTo(*s)?S::Ready:S::MismatchedDomain;
        return s->busy()?S::Busy:S::Ready;
    }
    TalkBackgroundState* Get(TalkBackgroundHandle identity) {
        if(!identity || !Live())return nullptr;
        const auto found=rows.find(identity->serial);
        return found!=rows.end() && found->second.identity==identity?&found->second:nullptr;
    }
};
struct NativeTalkBackground::Continuation final:ProcessContinuation {
    std::shared_ptr<State> state;ProcessCall call;TalkBackgroundHandle identity;
    std::string name;FilePathHandle path;std::size_t index{};unsigned stage{};TalkBackgroundDraw batch;
    ProcessCallbackStep Step(ProcessAccess& access) override {
        if(state->Mutable(&access)!=S::Ready)return ProcessCallbackStep::Blocked();
        auto* row=state->Get(identity);if(!row)return ProcessCallbackStep::Blocked();
        if(call.target==Load) {
            if(stage==0) {
                if(row->selected>=2)return ProcessCallbackStep::Blocked();
                auto file_name="face/"+name+".bch.lz";
                file_name.resize(std::min<std::size_t>(file_name.size(),63)); // original snprintf buffer64
                if(state->bases->RegisterPath(file_name,path,&access)!=FileBaseStatus::Ready)return ProcessCallbackStep::Blocked();
                const auto child=state->tex->ReadCall(call.process,row->files[row->selected],path,2,TexFileRead::Immediate);
                if(!child)return ProcessCallbackStep::Blocked();
                stage=1;return ProcessCallbackStep::Call(*child);
            }
            // The original re-reads selected AFTER nested transport/setup. Do not
            // accidentally reset the layer chosen at call entry when selection changed.
            if(row->selected>=2)return ProcessCallbackStep::Blocked();
            row->colors[row->selected]={255,255,255,0};return ProcessCallbackStep::Return();
        }
        if(call.target==Destroy) {
            for(;;) {
                if(stage==1){row->files[index].reset();++index;stage=0;}
                if(index==2)break;
                if(!row->files[index]){++index;continue;}
                const auto child=state->tex->DestroyCall(call.process,row->files[index],true);
                if(!child)return ProcessCallbackStep::Blocked();
                stage=1;return ProcessCallbackStep::Call(*child);
            }
            for(std::size_t reverse=2;reverse>0;--reverse)state->objects->Remove(row->color_handles[reverse-1]);
            state->objects->Remove(row->movable_handle);state->rows.erase(identity->serial);return ProcessCallbackStep::Return();
        }
        for(;;) {
            if(index==2) {
                // Revalidate borrowed textures before submitting retained pending output.
                for(const auto& rect:batch.rectangles) {
                    NativeTextureDescription description;
                    if(state->textures->Describe(rect.texture,description)!=TextureObjectStatus::Ready)return ProcessCallbackStep::Blocked();
                }
                if(state->sink && !state->sink->Submit(batch))return ProcessCallbackStep::Blocked();
                return ProcessCallbackStep::Return();
            }
            if(stage==0) {
                if(!row->files[index])return ProcessCallbackStep::Blocked();
                stage=1;return ProcessCallbackStep::Call(Service(call.process,0x545df4,{row->files[index]->serial}));
            }
            if(!access.call_result()){++index;stage=0;continue;}
            const auto texture=state->tex->GetTexture(row->files[index],std::int32_t{0});
            // The original getter normally returns a nonnull texture; dummy
            // texture geometry is not currently described by the accepted owner.
            // It is an explicit barrier, not silently omitted/null output.
            if(texture.status!=TextureObjectStatus::Ready)return ProcessCallbackStep::Blocked();
            NativeTextureDescription description;
            if(state->textures->Describe(texture.value,description)!=TextureObjectStatus::Ready)return ProcessCallbackStep::Blocked();
            const auto order=row->float_first?call.arguments[1]-static_cast<std::uint32_t>(index)+1u:
                call.arguments[1]+static_cast<std::uint32_t>(index);
            batch.rectangles.push_back({texture.value,row->positions[index],row->colors[index],description.width,
                description.height,std::bit_cast<std::int16_t>(static_cast<std::uint16_t>(order)),static_cast<std::uint32_t>(index)});
            ++index;stage=0;
        }
    }
};
NativeTalkBackground::NativeTalkBackground(std::shared_ptr<State> state):state_(std::move(state)){}
NativeTalkBackground::~NativeTalkBackground()=default;
S NativeTalkBackground::Create(std::shared_ptr<NativeProcessScheduler> scheduler,std::shared_ptr<ProcessCallbackRegistry> registry,
    std::shared_ptr<ObjectHandleRegistry> objects,std::shared_ptr<NativeFileController> files,std::shared_ptr<NativeFileBase> bases,
    std::shared_ptr<NativeTexFiles> tex,std::shared_ptr<NativeTextureObjects> textures,std::shared_ptr<TalkBackgroundSink> sink,
    std::shared_ptr<NativeTalkBackground>& out) {
    if(!scheduler || !scheduler->root(2))return S::NullScheduler;
    if(!registry || !scheduler->UsesCallbacks(registry.get()) || !objects || !files || !files->UsesScheduler(*scheduler) ||
        !bases || !bases->UsesController(*files) || !tex || !tex->UsesOwners(*files,*bases) || !textures || !tex->UsesTextureObjects(*textures))return S::MismatchedDomain;
    auto state=std::make_shared<State>();state->scheduler=scheduler;state->objects=std::move(objects);state->files=std::move(files);
    state->bases=std::move(bases);state->tex=std::move(tex);state->textures=std::move(textures);state->sink=std::move(sink);
    auto owner=std::shared_ptr<NativeTalkBackground>(new NativeTalkBackground(state));
    if(!registry->Register(Targets,owner))return S::DuplicateBinding;
    out=std::move(owner);return S::Ready;
}
S NativeTalkBackground::Construct(TalkBackgroundHandle& out,ProcessAccess* access) {
    if(auto result=state_->Mutable(access);result!=S::Ready)return result;
    if(state_->serial==std::numeric_limits<std::uint32_t>::max())return S::IdentityExhausted;
    TalkBackgroundState row;row.identity=std::make_shared<const TalkBackgroundIdentity>(++state_->serial);
    row.movable_identity=state_->objects->NewIdentity();state_->objects->Entry(row.movable_handle,row.movable_identity);
    for(std::size_t i=0;i<2;++i){row.color_identities[i]=state_->objects->NewIdentity();state_->objects->Entry(row.color_handles[i],row.color_identities[i]);}
    for(std::size_t i=0;i<2;++i) {
        row.colors[i]={255,255,255,0};
        if(state_->tex->Construct(row.files[i],access)!=TexFileStatus::Ready) {
            for(const auto& file:row.files)if(file)state_->bases->RetireEmpty(file,access);
            for(std::size_t reverse=2;reverse>0;--reverse)state_->objects->Remove(row.color_handles[reverse-1]);
            state_->objects->Remove(row.movable_handle);return S::Unavailable;
        }
    }
    state_->rows.emplace(row.identity->serial,row);out=row.identity;return S::Ready;
}
S NativeTalkBackground::RegisterName(std::string_view input,TalkBackgroundName& out,ProcessAccess* access) {
    if(auto result=state_->Mutable(access);result!=S::Ready)return result;
    if(state_->name_serial==std::numeric_limits<std::uint32_t>::max())return S::IdentityExhausted;
    const auto zero=input.find('\0');auto value=std::string(input.substr(0,std::min<std::size_t>(zero==std::string_view::npos?input.size():zero,63)));
    auto id=std::make_shared<const TalkBackgroundNameIdentity>(++state_->name_serial);
    state_->names.emplace(id->serial,State::Name{id,std::move(value)});out=id;return S::Ready;
}
S NativeTalkBackground::Select(TalkBackgroundHandle handle,std::uint32_t value,ProcessAccess* access) {
    if(auto result=state_->Mutable(access);result!=S::Ready)return result;
    auto* row=state_->Get(handle);if(!row)return S::InvalidHandle;
    if(value>=2)return S::InvalidSelection;
    row->selected=value;return S::Ready;
}
S NativeTalkBackground::FloatSelected(TalkBackgroundHandle handle,ProcessAccess* access) {
    if(auto result=state_->Mutable(access);result!=S::Ready)return result;
    auto* row=state_->Get(handle);if(!row)return S::InvalidHandle;
    row->float_first=row->selected==0?1:0;return S::Ready;
}
S NativeTalkBackground::SetPosition(TalkBackgroundHandle handle,std::array<std::uint32_t,3> value,ProcessAccess* access) {
    if(auto result=state_->Mutable(access);result!=S::Ready)return result;
    auto* row=state_->Get(handle);if(!row)return S::InvalidHandle;
    row->positions[row->selected]=value;return S::Ready;
}
S NativeTalkBackground::SetPositionX(TalkBackgroundHandle handle,std::uint32_t value,ProcessAccess* access) {
    if(auto result=state_->Mutable(access);result!=S::Ready)return result;
    auto* row=state_->Get(handle);if(!row)return S::InvalidHandle;
    row->positions[row->selected][0]=value;return S::Ready;
}
S NativeTalkBackground::SetColor(TalkBackgroundHandle handle,std::size_t slot,std::array<std::uint8_t,4> value,ProcessAccess* access) {
    if(auto result=state_->Mutable(access);result!=S::Ready)return result;
    auto* row=state_->Get(handle);if(!row)return S::InvalidHandle;
    if(slot>=2)return S::InvalidSelection;
    row->colors[slot]=value;return S::Ready;
}
std::optional<TalkBackgroundState> NativeTalkBackground::Observe(TalkBackgroundHandle handle) const {
    const auto* row=state_->Get(handle);return row?std::optional{*row}:std::nullopt;
}
std::optional<std::array<std::uint32_t,3>> NativeTalkBackground::Position(TalkBackgroundHandle handle) const {
    const auto* row=state_->Get(handle);return row?std::optional{row->positions[row->selected]}:std::nullopt;
}
std::optional<ProcessCall> NativeTalkBackground::LoadCall(ProcessHandle process,TalkBackgroundHandle handle,TalkBackgroundName name) const {
    if(!state_->Get(handle) || !name)return {};
    const auto found=state_->names.find(name->serial);
    if(found==state_->names.end() || found->second.identity!=name)return {};
    return Service(std::move(process),Load,{handle->serial,name->serial});
}
std::optional<ProcessCall> NativeTalkBackground::DrawCall(ProcessHandle process,TalkBackgroundHandle handle,std::uint16_t priority) const {
    if(!state_->Get(handle))return {};
    return Service(std::move(process),Draw,{handle->serial,priority});
}
std::optional<ProcessCall> NativeTalkBackground::DestroyCall(ProcessHandle process,TalkBackgroundHandle handle) const {
    if(!state_->Get(handle))return {};
    return Service(std::move(process),Destroy,{handle->serial});
}
bool NativeTalkBackground::UsesObjectRegistry(const ObjectHandleRegistry& registry) const noexcept{return state_->objects.get()==&registry;}
std::unique_ptr<ProcessContinuation> NativeTalkBackground::Begin(const ProcessCall& call) {
    if(!state_->Live() || call.kind!=ProcessCallKind::Service || call.has_self || call.this_adjustment || !call.process ||
        std::find(Targets.begin(),Targets.end(),call.target)==Targets.end() || call.argument_count!=(call.target==Destroy?1u:2u))return {};
    const auto found=state_->rows.find(call.arguments[0]);if(found==state_->rows.end())return {};
    auto next=std::make_unique<Continuation>();next->state=state_;next->call=call;next->identity=found->second.identity;
    if(call.target==Load){const auto name=state_->names.find(call.arguments[1]);if(name==state_->names.end())return {};next->name=name->second.value;}
    if(call.target==Draw)next->call.arguments[1]&=0xffffu;
    return next;
}
}

namespace fates::presentation::native {
bool NativeTalkBackground::UsesScheduler(const runtime::native::NativeProcessScheduler& s) const noexcept{return state_->scheduler.lock().get()==&s;}
std::optional<TalkColorBytes> NativeTalkBackground::ReadColor(runtime::native::ObjectIdentity identity) const {
    if(!identity || !state_->Live())return {};
    for(const auto& [key,row]:state_->rows){(void)key;for(unsigned i=0;i<2;++i)if(row.color_identities[i]==identity && state_->objects->Get(row.color_handles[i])==identity)return row.colors[i];}
    return {};
}
bool NativeTalkBackground::WriteColorChannel(runtime::native::ObjectIdentity identity,std::uint8_t channel,std::uint8_t value,runtime::native::ProcessAccess& access) {
    if(channel>=4 || !identity || state_->Mutable(&access)!=TalkBackgroundStatus::Ready)return false;
    for(auto& [key,row]:state_->rows){(void)key;for(unsigned i=0;i<2;++i)if(row.color_identities[i]==identity && state_->objects->Get(row.color_handles[i])==identity){row.colors[i][channel]=value;return true;}}
    return false;
}
}

namespace fates::presentation::native {
std::optional<TalkVectorBits> NativeTalkBackground::ReadPosition(runtime::native::ObjectIdentity identity) const {
    if(!identity || !state_->Live())return {};
    for(const auto& [key,row]:state_->rows){(void)key;if(row.movable_identity==identity && state_->objects->Get(row.movable_handle)==identity)return row.movable_position;}
    return {};
}
bool NativeTalkBackground::WritePosition(runtime::native::ObjectIdentity identity,TalkVectorBits value,runtime::native::ProcessAccess& access) {
    if(!identity || state_->Mutable(&access)!=TalkBackgroundStatus::Ready)return false;
    for(auto& [key,row]:state_->rows){(void)key;if(row.movable_identity==identity && state_->objects->Get(row.movable_handle)==identity){row.movable_position=value;return true;}}
    return false;
}
}
