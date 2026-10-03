#include "fates/presentation/native_talk_font_effects.hpp"
namespace fates::presentation::native {
using namespace runtime::native;
struct NativeTalkFontEffects::Continuation final:ProcessContinuation {
    std::weak_ptr<NativeProcessScheduler> scheduler;
    std::shared_ptr<NativeTalkWindow> windows;std::shared_ptr<NativeFontMetrics> fonts;
    ProcessCall call;TalkWindowHandle window;TalkColorBytes color{};unsigned stage{};std::uint32_t saved{},width{};
    FontTextRequest request;
    ~Continuation() override {if(request)fonts->ForgetText(request);}
    ProcessCallbackStep Step(ProcessAccess& access) override {
        const auto owner=scheduler.lock();if(!owner || !access.BelongsTo(*owner))return ProcessCallbackStep::Blocked();
        if(stage==0){if(windows->WriteFontColor(window,color,access)!=TalkWindowStatus::Ready)return ProcessCallbackStep::Blocked();stage=1;}
        if(stage==1){stage=2;return ProcessCallbackStep::Call(NativeFontMetrics::GetCurrentCall(call.process));}
        if(stage==2){saved=access.call_result();stage=3;}
        if(stage==3){const auto row=windows->Observe(window);if(!row)return ProcessCallbackStep::Blocked();stage=4;return ProcessCallbackStep::Call(NativeFontMetrics::SetCurrentCall(call.process,row->font));}
        if(stage==4){
            const auto row=windows->Observe(window);if(!row || row->current>=8)return ProcessCallbackStep::Blocked();
            TalkWindowSource source;source.kind=TalkWindowSource::Kind::WindowString;source.string_view={window,static_cast<std::uint8_t>(row->current)};
            const auto storage=windows;
            if(fonts->PrepareText([storage,source](std::size_t index)->std::optional<char16_t>{const auto value=storage->Read(source,index);return value.status==TalkWindowStatus::Ready?std::optional{value.value}:std::nullopt;},FontTextOperation::Width,request,&access)!=FontMetricStatus::Ready)return ProcessCallbackStep::Blocked();
            stage=5;
        }
        if(stage==5){const auto child=fonts->TextCall(call.process,request);if(!child)return ProcessCallbackStep::Blocked();stage=6;return ProcessCallbackStep::Call(*child);}
        if(stage==6){width=access.call_result();if(fonts->Release(request,&access)!=FontMetricStatus::Ready)return ProcessCallbackStep::Blocked();request.reset();stage=7;}
        if(stage==7){if(windows->AddMeasuredXOffset(window,width,access)!=TalkWindowStatus::Ready)return ProcessCallbackStep::Blocked();stage=8;}
        if(stage==8){if(windows->NextString(window,&access)!=TalkWindowStatus::Ready)return ProcessCallbackStep::Blocked();stage=9;return ProcessCallbackStep::Call(NativeFontMetrics::SetCurrentCall(call.process,saved));}
        return ProcessCallbackStep::Return();
    }
};
FontMetricStatus NativeTalkFontEffects::Create(std::shared_ptr<NativeProcessScheduler> scheduler,std::shared_ptr<ProcessCallbackRegistry> registry,
    std::shared_ptr<NativeTalkWindow> windows,std::shared_ptr<NativeFontMetrics> fonts,std::shared_ptr<NativeTalkFontEffects>& out){
    if(!scheduler || !scheduler->root(2))return FontMetricStatus::NullScheduler;
    if(!registry || !scheduler->UsesCallbacks(registry.get()) || !windows || !windows->UsesScheduler(*scheduler) || !fonts || !fonts->UsesScheduler(*scheduler))return FontMetricStatus::MismatchedDomain;
    auto next=std::shared_ptr<NativeTalkFontEffects>(new NativeTalkFontEffects);next->scheduler_=scheduler;next->windows_=std::move(windows);next->fonts_=std::move(fonts);
    if(!registry->Register(std::array{std::uint32_t{0x3cf9b4}},next))return FontMetricStatus::DuplicateBinding;
    out=std::move(next);return FontMetricStatus::Ready;
}
ProcessCall NativeTalkFontEffects::ChangeColorCall(ProcessHandle process,TalkWindowHandle window,TalkColorBytes color){
    ProcessCall call;call.process=std::move(process);call.target=0x3cf9b4;call.kind=ProcessCallKind::Service;call.argument_count=2;
    call.arguments[0]=window?window->serial:0;for(std::size_t index=0;index<4;++index)call.arguments[1]|=std::uint32_t(color[index])<<(8*index);return call;
}
bool NativeTalkFontEffects::UsesWindow(const NativeTalkWindow& windows)const noexcept{return windows_.get()==&windows;}
bool NativeTalkFontEffects::UsesScheduler(const NativeProcessScheduler& s)const noexcept{return scheduler_.lock().get()==&s;}
std::unique_ptr<ProcessContinuation> NativeTalkFontEffects::Begin(const ProcessCall& call){
    const auto scheduler=scheduler_.lock();if(!scheduler || !scheduler->root(2) || !call.process || call.kind!=ProcessCallKind::Service || call.target!=0x3cf9b4 || call.has_self || call.this_adjustment || call.argument_count!=2)return {};
    auto next=std::make_unique<Continuation>();next->scheduler=scheduler_;next->windows=windows_;next->fonts=fonts_;next->call=call;
    for(const auto& window:windows_->Handles())if(window->serial==call.arguments[0])next->window=window;
    if(!next->window)return {};
    for(std::size_t index=0;index<4;++index)next->color[index]=static_cast<std::uint8_t>(call.arguments[1]>>(8*index));
    return next;
}
}
