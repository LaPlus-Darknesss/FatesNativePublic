#include "fates/presentation/native_talk_speaker.hpp"

namespace fates::presentation::native {
using namespace runtime::native;
using S=TalkSpeakerStatus;
namespace {
TalkWindowSource Name(TalkWindowHandle window,const TalkNamePlateStorage& plate) {
    if(!plate.name)return {};
    TalkWindowSource result;result.kind=TalkWindowSource::Kind::Name;result.name_view={std::move(window),plate.name,0};return result;
}
}
S NativeTalkSpeaker::Create(std::shared_ptr<NativeProcessScheduler> scheduler,std::shared_ptr<NativeTalkWindow> windows,
    std::shared_ptr<NativeUnitNames> names,std::shared_ptr<NativeMessageLookup> messages,std::shared_ptr<NativeTalkSpeaker>& output) {
    if(!scheduler || !scheduler->root(2))return S::NullScheduler;
    if(!windows || !windows->UsesScheduler(*scheduler) || !names || !messages || !names->UsesMessages(*messages))return S::MismatchedDomain;
    auto result=std::shared_ptr<NativeTalkSpeaker>(new NativeTalkSpeaker());
    result->scheduler_=scheduler;result->windows_=std::move(windows);result->names_=std::move(names);result->messages_=std::move(messages);
    output=std::move(result);return S::Ready;
}
TalkSpeakerResult NativeTalkSpeaker::Get(TalkWindowHandle handle,ProcessAccess* access) {
    const auto scheduler=scheduler_.lock();if(!scheduler || !scheduler->root(2))return {S::Retired};
    if(access){if(!access->BelongsTo(*scheduler))return {S::MismatchedDomain};}
    else if(scheduler->busy())return {S::Busy};
    const auto row=windows_->Observe(handle);if(!row)return {S::InvalidWindow};
    if(row->name_override)return {S::Ready,TalkSpeakerOrigin::ExplicitName,UnitNameStatus::Ready,Name(handle,row->name_plate)};
    std::string fid;
    for(std::size_t index=0;index<32;++index) {
        // Same +5DC byte read by name-fader admission. Known null terminates
        // immediately; unknown trailing bytes are not read after a terminator.
        const auto byte=index? (row->face_identifier_known.test(index-1)?std::optional<std::uint8_t>{row->face_identifier_tail[index-1]}:std::nullopt):row->name_effect_enabled;
        if(!byte)return {S::UnknownIdentifier};
        if(!*byte)break;
        fid.push_back(static_cast<char>(*byte));
    }
    // If all32bytes are nonzero, the already-read override byte at+5FC is zero
    // in this branch and is the exact adjacent C-string terminator. Do not
    // truncate to31 or require an invented terminator inside the field.
    for(std::int32_t type:{1,0}) {
        auto face=names_->FindFace(fid,type);
        if(face.status!=UnitNameStatus::Ready)return {S::NameUnavailable,TalkSpeakerOrigin::FaceMessage,face.status};
        if(face.value.registration) {
            auto result=names_->GetFaceMessage(face.value);
            if(result.status!=UnitNameStatus::Ready)return {S::NameUnavailable,TalkSpeakerOrigin::FaceMessage,result.status};
            if(result.edit_source) {
                TalkWindowSource live;live.kind=TalkWindowSource::Kind::UnitEdit;live.unit_names=names_;live.edit_name_source=*result.edit_source;
                return {S::Ready,TalkSpeakerOrigin::FaceMessage,UnitNameStatus::Ready,std::move(live)};
            }
            return {S::Ready,TalkSpeakerOrigin::FaceMessage,UnitNameStatus::Ready,TalkWindowSource::Message(messages_,std::move(result.message))};
        }
    }
    if(row->strings_visible && row->name_plate.name)return {S::Ready,TalkSpeakerOrigin::RetainedName,UnitNameStatus::Ready,Name(handle,row->name_plate)};
    // Exact executable UTF16 literal at507A4C, not a localized/empty substitute.
    static const auto fallback=TalkWindowSource::Text(u"\u540d\u7121\u3057");
    return {S::Ready,TalkSpeakerOrigin::OriginalFallback,UnitNameStatus::Ready,fallback};
}
}
