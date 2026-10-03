#include "fates/presentation/native_font_startup.hpp"
#include <algorithm>
#include <limits>

namespace fates::presentation::native {
using namespace runtime::native;
using namespace io::native;
namespace {
constexpr std::uint32_t Initialize=0x108344,DestroyStaticSlots=0x163c00;
// PROVEN: original name table at 0x006DB6EC. The null third entry skips
// loading altogether; it does not clear a previously populated slot.
constexpr std::array<const char*,4> Names{"System.bfnt.lz","System.bfnt.lz",nullptr,"Dummy.bfnt.lz"};
// PROVEN: twelve RGBA writes at 0x005BC50C to Font's own 0x006DB6AC
// table. These are not TalkWindowPalette's separate three globals.
constexpr FontStaticColors Colors{{{255,255,255,255},{0,0,0,0},{255,255,255,255},
    {0,0,0,255},{255,0,0,255},{0,255,0,255},{0,0,255,255},{255,255,0,255},
    {255,0,255,255},{0,255,255,255},{128,128,128,255},{64,64,64,255}}};
}
struct NativeFontStartup::State {
    std::weak_ptr<NativeProcessScheduler> scheduler;
    std::shared_ptr<NativeFileBase> bases;
    std::shared_ptr<NativeFontMetrics> metrics;
    std::shared_ptr<NativeFontFiles> loader;
    FontStartupObservation value;
    std::array<std::optional<FontBufferObservation>,8> buffers;
    bool Live() const {const auto owner=scheduler.lock();return owner && owner->root(2);}
    FontStartupStatus Mutable(ProcessAccess* access) const {
        const auto owner=scheduler.lock();if(!owner || !owner->root(2) || value.static_lifetime==FontStaticLifetime::Destroyed)return FontStartupStatus::Retired;
        if(access)return access->BelongsTo(*owner)?FontStartupStatus::Ready:FontStartupStatus::MismatchedDomain;
        return owner->busy()?FontStartupStatus::Busy:FontStartupStatus::Ready;
    }
};
struct NativeFontStartup::Continuation final:ProcessContinuation {
    std::shared_ptr<State> state;ProcessCall call;unsigned slot{},buffer{},stage{};
    FilePathHandle name;
    ProcessCallbackStep Step(ProcessAccess& access) override {
        const auto owner=state->scheduler.lock();
        if(!owner || !access.BelongsTo(*owner))return ProcessCallbackStep::Blocked();
        if(call.target==DestroyStaticSlots) {
            if(!state->value.static_slots)return ProcessCallbackStep::Blocked();
            state->value.static_lifetime=FontStaticLifetime::Destroying;
            while(slot<4) {
                const auto base=(*state->value.static_slots)[3-slot];
                if(stage==0) {
                    const auto child=state->bases->FreeCall(call.process,base);
                    if(!child)return ProcessCallbackStep::Blocked();
                    stage=1;return ProcessCallbackStep::Call(*child);
                }
                if(state->bases->RetireEmpty(base,&access)!=FileBaseStatus::Ready)return ProcessCallbackStep::Blocked();
                ++slot;++state->value.destroyed_slots;stage=0;
            }
            // Original exit registration for the buffer/stack destructors is
            // patched to NOP. Their payload is not cleared by this destructor.
            state->value.static_lifetime=FontStaticLifetime::Destroyed;
            return ProcessCallbackStep::Return();
        }
        if(stage==0) {
            while(slot<Names.size()) {
                if(!Names[slot]){++slot;continue;}
                if(!name && state->bases->RegisterPath(Names[slot],name,&access)!=FileBaseStatus::Ready)
                    return ProcessCallbackStep::Blocked();
                const auto child=state->loader->LoadCall(call.process,slot,name);
                if(!child)return ProcessCallbackStep::Blocked();
                // The nested loader retains Open/Entry/Close and final selector
                // work. Advancing here cannot replay that prefix on suspension.
                ++slot;name.reset();return ProcessCallbackStep::Call(*child);
            }
            stage=1;
        }
        if(stage==1) {
            for(;buffer<state->buffers.size();++buffer) {
                auto& row=state->buffers[buffer];if(!row)return ProcessCallbackStep::Blocked();
                if(row->capacity<256) {
                    const auto size=static_cast<std::uint32_t>(row->records.size());
                    const auto capacity=std::max({256u,size+32u,size+(size>>1u)+(size>>3u)});
                    row->records.reserve(capacity);row->capacity=capacity;
                }
            }
            stage=2;
        }
        if(stage==2) {
            if(state->metrics->ReserveStartupStack(access)!=FontMetricStatus::Ready)return ProcessCallbackStep::Blocked();
            stage=3;return ProcessCallbackStep::Call(NativeFontMetrics::ResetStackCall(call.process));
        }
        if(stage==3) {
            stage=4;return ProcessCallbackStep::Call(NativeFontMetrics::SetCurrentCall(call.process,0));
        }
        ++state->value.completed_initializations;return ProcessCallbackStep::Return();
    }
};
NativeFontStartup::NativeFontStartup(std::shared_ptr<State> state):state_(std::move(state)){}
NativeFontStartup::~NativeFontStartup()=default;
FontStartupStatus NativeFontStartup::Create(std::shared_ptr<NativeProcessScheduler> scheduler,
    std::shared_ptr<ProcessCallbackRegistry> registry,std::shared_ptr<NativeFileBase> bases,
    std::shared_ptr<NativeFontMetrics> metrics,std::shared_ptr<NativeFontObjects> objects,
    std::shared_ptr<NativeFontFiles> loader,std::shared_ptr<NativeFontStartup>& output) {
    if(!scheduler || !scheduler->root(2))return FontStartupStatus::NullScheduler;
    if(!registry || !scheduler->UsesCallbacks(registry.get()) || !bases || !metrics || !objects || !loader ||
       !metrics->UsesScheduler(*scheduler) || !loader->UsesOwners(*bases,*metrics,*objects))return FontStartupStatus::MismatchedDomain;
    auto state=std::make_shared<State>();state->scheduler=scheduler;state->bases=std::move(bases);
    state->metrics=std::move(metrics);state->loader=std::move(loader);
    auto module=std::shared_ptr<NativeFontStartup>(new NativeFontStartup(state));
    const std::array targets{Initialize,DestroyStaticSlots};
    if(!registry->Register(targets,module))return FontStartupStatus::DuplicateBinding;
    output=std::move(module);return FontStartupStatus::Ready;
}
FontStartupStatus NativeFontStartup::ConstructFreshStorage(ProcessAccess* access) {
    if(const auto status=state_->Mutable(access);status!=FontStartupStatus::Ready)return status;
    if(state_->value.static_lifetime!=FontStaticLifetime::Carried || state_->value.completed_initializations ||
       std::any_of(state_->buffers.begin(),state_->buffers.end(),[](const auto& row){return row.has_value();}) ||
       !state_->metrics->CanConstructStaticSelection())return FontStartupStatus::InvalidState;
    std::array<FileBaseHandle,4> slots;
    const auto retire=[&](){for(const auto& base:slots)if(base)state_->bases->RetireEmpty(base,access);};
    for(auto& base:slots)if(state_->bases->RestoreAttachment({},base,access)!=FileBaseStatus::Ready) {
        retire();return FontStartupStatus::Unavailable;
    }
    // The selector byte 4 and null pointer come from the executable image,
    // not writes in __sti___8_Font_cpp. Publish its constructed stack/slots
    // through the one existing selector owner; never call SetCurrent here.
    const auto before=*state_->metrics->Selection();FontSelectionState selection;
    for(auto& slot:selection.slots)slot=FileObjectHandle{};
    selection.current_type=std::uint8_t{4};selection.current=FileObjectHandle{};
    selection.stack=std::vector<std::uint8_t>{};selection.stack_capacity=0;
    if(state_->metrics->RestoreSelection(selection,access)!=FontMetricStatus::Ready) {
        retire();return FontStartupStatus::Unavailable;
    }
    if(state_->loader->BindSlots(slots,access)!=FontFileStatus::Ready) {
        state_->metrics->RestoreSelection(before,access);retire();return FontStartupStatus::InvalidState;
    }
    for(auto& buffer:state_->buffers)buffer=FontBufferObservation{};
    state_->value.colors=Colors;state_->value.static_slots=std::move(slots);
    state_->value.static_lifetime=FontStaticLifetime::Constructed;
    return FontStartupStatus::Ready;
}
FontStartupStatus NativeFontStartup::RestoreBuffer(std::uint32_t index,const FontStartupBuffer& value,ProcessAccess* access) {
    if(const auto status=state_->Mutable(access);status!=FontStartupStatus::Ready)return status;
    if(index>=8 || value.records.size()>value.capacity || value.capacity>std::numeric_limits<std::uint32_t>::max()/8)
        return FontStartupStatus::InvalidState;
    FontBufferObservation restored;restored.capacity=value.capacity;restored.records.reserve(value.capacity);
    for(const auto& record:value.records)restored.records.emplace_back(record);
    state_->buffers[index]=std::move(restored);
    return FontStartupStatus::Ready;
}
std::optional<FontStartupObservation> NativeFontStartup::Observe() const {
    if(!state_->Live())return {};
    auto result=state_->value;
    for(std::size_t i=0;i<state_->buffers.size();++i) {
        const auto& source=state_->buffers[i];if(!source)continue;
        FontStartupBuffer row;row.capacity=source->capacity;bool opaque=true;
        for(const auto& record:source->records) {
            const auto bytes=std::get_if<std::array<std::uint8_t,8>>(&record);
            if(!bytes){opaque=false;break;}row.records.push_back(*bytes);
        }
        if(opaque)result.buffers[i]=std::move(row);
    }
    return result;
}
std::optional<FontBufferObservation> NativeFontStartup::ObserveBuffer(std::uint32_t index) const {
    return state_->Live() && index<8?state_->buffers[index]:std::nullopt;
}
bool NativeFontStartup::UsesOwners(const NativeFontMetrics& metrics,const NativeFontObjects& objects) const noexcept {
    return state_->metrics.get()==&metrics && state_->loader->UsesOwners(*state_->bases,metrics,objects);
}
FontStartupStatus NativeFontStartup::ClearDrawBuffer(std::uint32_t index,ProcessAccess& access) {
    if(const auto status=state_->Mutable(&access);status!=FontStartupStatus::Ready)return status;
    if(index>=8)return FontStartupStatus::InvalidState;
    auto& row=state_->buffers[index];if(!row)return FontStartupStatus::Unavailable;
    row->records.clear();return FontStartupStatus::Ready;
}
FontStartupStatus NativeFontStartup::AppendDrawGlyph(std::uint32_t index,const FontQueuedGlyph& glyph,bool& stored,ProcessAccess& access) {
    if(const auto status=state_->Mutable(&access);status!=FontStartupStatus::Ready)return status;
    if(index>=8)return FontStartupStatus::InvalidState;
    auto& row=state_->buffers[index];if(!row)return FontStartupStatus::Unavailable;
    // PROVEN: 0x005062AC skips a full buffer before the generic insertion
    // fallback. The original still advances x for the skipped glyph.
    stored=row->records.size()<row->capacity;
    if(stored)row->records.emplace_back(glyph);
    return FontStartupStatus::Ready;
}
std::optional<std::size_t> NativeFontStartup::DrawBufferSize(std::uint32_t index) const {
    if(!state_->Live() || index>=8 || !state_->buffers[index])return {};
    return state_->buffers[index]->records.size();
}
std::optional<FontQueuedGlyph> NativeFontStartup::DrawGlyph(std::uint32_t index,std::size_t at) const {
    if(!state_->Live() || index>=8 || !state_->buffers[index] || at>=state_->buffers[index]->records.size())return {};
    const auto* glyph=std::get_if<FontQueuedGlyph>(&state_->buffers[index]->records[at]);
    return glyph?std::optional{*glyph}:std::nullopt;
}
ProcessCall NativeFontStartup::InitializeCall(ProcessHandle process) {
    ProcessCall call;call.process=std::move(process);call.kind=ProcessCallKind::Service;call.target=Initialize;return call;
}
ProcessCall NativeFontStartup::DestroyStaticSlotsCall(ProcessHandle process) {
    ProcessCall call;call.process=std::move(process);call.kind=ProcessCallKind::Service;call.target=DestroyStaticSlots;return call;
}
std::unique_ptr<ProcessContinuation> NativeFontStartup::Begin(const ProcessCall& call) {
    if(!state_->Live() || !call.process || call.kind!=ProcessCallKind::Service ||
       (call.target!=Initialize && call.target!=DestroyStaticSlots) ||
       call.argument_count || call.has_self || call.this_adjustment)return {};
    const auto life=state_->value.static_lifetime;
    if(life==FontStaticLifetime::Destroying || life==FontStaticLifetime::Destroyed ||
       (call.target==DestroyStaticSlots && life!=FontStaticLifetime::Constructed))return {};
    auto next=std::make_unique<Continuation>();next->state=state_;next->call=call;return next;
}
}
