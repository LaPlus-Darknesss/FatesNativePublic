#include "fates/runtime/native_game_user_settings.hpp"
#include "fates/runtime/native_runtime.hpp"
#include "fates/runtime/native_unit_transfer.hpp"
#include "fates/campaign/native_campaign_continuity.hpp"
#include <limits>
#include <memory>
namespace fates::runtime::native {
namespace cn=fates::campaign::native;
GameUserSettingsStatus RestoreGameUserDifficulty(NativeRuntime& r,GameUserDifficultySnapshot input) noexcept {
    auto& state=r.game.game_user_difficulty;
    if(state.bound)return GameUserSettingsStatus::AlreadyBound;
    if(input.value>2)return GameUserSettingsStatus::InvalidValue;
    if(state.revision==std::numeric_limits<std::uint64_t>::max())return GameUserSettingsStatus::RevisionExhausted;
    state.bound=true;state.value=input.value;++state.revision;return GameUserSettingsStatus::Ok;
}
GameUserSettingsStatus InvalidateGameUserDifficulty(NativeRuntime& r) noexcept {
    auto& state=r.game.game_user_difficulty;
    if(state.revision==std::numeric_limits<std::uint64_t>::max())return GameUserSettingsStatus::RevisionExhausted;
    state.bound=false;++state.revision;return GameUserSettingsStatus::Ok;
}
bool GameUserDifficultyMatches(const NativeRuntime& r,std::uint8_t scoped) noexcept {
    return scoped<=2&&(!r.game.game_user_difficulty.bound||r.game.game_user_difficulty.value==scoped);
}
std::optional<std::uint8_t> CurrentGameUserDifficulty(const NativeRuntime& r) noexcept {
    const auto& state=r.game.game_user_difficulty;
    if(!state.bound||state.value>2)return std::nullopt;return state.value;
}
std::uint32_t DifficultyChoiceAttributesExact(std::uint8_t current,std::uint8_t pending,std::uint8_t choice) noexcept {
    return (choice==pending?0x10u:0u)|(choice>current?2u:1u);
}
std::uint32_t ModeChoiceAttributesExact(std::uint32_t flags,std::uint8_t difficulty,std::uint8_t pending,std::uint8_t choice) noexcept {
    if(choice==0&&difficulty!=0)return 4;
    const auto selected=choice==pending?0x10u:0u;
    return selected|((choice==2&&cn::GetModeFromBits(flags)!=2)?2u:1u);
}
GameUserChoiceResult SelectGameUserChoiceExact(GameUserPendingChoice pending,std::uint32_t attributes,std::uint8_t choice,bool mode) noexcept {
    if(attributes&2u)return {pending,0x1000,-1};
    if(mode)pending.mode=choice;else pending.difficulty=choice;
    return {pending,0x108,mode?2:1};
}
GameUserSettingsChangePlan PlanGameUserSettingsChangeExact(std::uint32_t flags,GameUserPendingChoice pending,std::span<const std::uint32_t> force) {
    GameUserSettingsChangePlan plan{};plan.flags=cn::SetModeBits(flags,pending.mode);plan.difficulty=pending.difficulty;
    plan.transfer_force4_to3=cn::GetModeFromBits(flags)==2&&pending.mode!=2;
    plan.force4_flags.assign(force.begin(),force.end());
    if(plan.transfer_force4_to3)for(auto& value:plan.force4_flags)value&=~0x20088u;
    return plan;
}
GameUserSettingsStatus ChangeGameUserSettings(NativeRuntime& r,GameUserPendingChoice pending) {
    auto& state=r.game.game_user_difficulty;auto& flags=r.game.campaign.game_user_flags;
    if(!state.bound)return GameUserSettingsStatus::Unbound;
    if(state.value>2||pending.difficulty>2||pending.mode>2)return GameUserSettingsStatus::InvalidValue;
    if((DifficultyChoiceAttributesExact(state.value,state.value,pending.difficulty)&2u)||
       (ModeChoiceAttributesExact(flags,pending.difficulty,std::uint8_t(cn::GetModeFromBits(flags)),pending.mode)&6u))return GameUserSettingsStatus::Disabled;
    if(state.revision==std::numeric_limits<std::uint64_t>::max())return GameUserSettingsStatus::RevisionExhausted;
    if(cn::GetModeFromBits(flags)==2&&pending.mode!=2) {
        auto image=std::make_unique<NativeRuntime>(r);
        image->game.campaign.game_user_flags=cn::SetModeBits(flags,pending.mode);
        image->game.game_user_difficulty.value=pending.difficulty;++image->game.game_user_difficulty.revision;
        for(auto& u:image->game.units)if(u.occupied&&u.force_type==4)u.flags&=~0x20088u;
        if(TransferForceUnits(*image,4,3,true).status!=UnitTransferStatus::Ok)return GameUserSettingsStatus::ForceTransferRequired;
        r.game=std::move(image->game);return GameUserSettingsStatus::Ok;
    }
    flags=cn::SetModeBits(flags,pending.mode);state.value=pending.difficulty;++state.revision;return GameUserSettingsStatus::Ok;
}
}
