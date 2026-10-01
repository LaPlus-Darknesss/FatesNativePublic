#pragma once
#include <cstddef>
#include <cstdint>
#include <string_view>

namespace fates::event::backend {

using BackendWord = std::intptr_t;

enum class BackendFamily : std::uint8_t { CoreState, ProcessUi, MapField, GameplayData, Presentation, Castle, Infrastructure };

struct BackendSpec {
    const char* retail_symbol;
    std::uint32_t retail_address;
    const char* retail_signature;
    BackendFamily family;
    std::uint16_t direct_event_references;
};

struct BackendRuntime {
    using InvokeFn = BackendWord (*)(void* user, const BackendSpec& spec, const BackendWord* args, std::size_t argc);
    void* user = nullptr;
    InvokeFn invoke = nullptr;
};

const BackendSpec* GetBackendSpecs();
std::size_t GetBackendSpecCount();
const BackendSpec* FindBackendSpec(std::uint32_t retail_address);
BackendWord InvokeBackend(BackendRuntime&, std::uint32_t retail_address, const BackendWord* args, std::size_t argc);

// Proven layout-independent policies recovered directly from retail.
bool GameSkipStateIsBlackOut(std::uint8_t state);
bool GameSkipStateIsSkipping(std::uint8_t state);

struct NamedVariableTableView { const char* const* names; std::int32_t* values; std::size_t count; };
std::int32_t VariableFind(NamedVariableTableView table, std::string_view name);
bool VariableSet(NamedVariableTableView table, std::string_view name, std::int32_t value);
bool VariableAdd(NamedVariableTableView table, std::string_view name, std::int32_t delta);
std::int32_t VariableGet(NamedVariableTableView table, std::string_view name);

struct FlagNameTableView { const char** names; std::size_t count; };
std::int32_t FlagEntryForward(FlagNameTableView table, const char* name);
std::int32_t FlagEntryReverse(FlagNameTableView table, const char* name);
bool FlagTest(const std::uint8_t* bits, std::size_t bit_count, std::int32_t index);
bool FlagSet(std::uint8_t* bits, std::size_t bit_count, std::int32_t index);
bool FlagClear(std::uint8_t* bits, std::size_t bit_count, std::int32_t index);

struct ContentTimestampFields { std::uint16_t year; std::uint8_t month; std::uint8_t day; std::uint8_t hour; std::uint8_t minute; };
std::uint32_t PackContentTimestamp(ContentTimestampFields fields);
ContentTimestampFields UnpackContentTimestamp(std::uint32_t packed);
bool SetFieldObjectEscapeBits(std::uint16_t& flags, bool escape);

// One source-facing wrapper per newly owned retail backend function. Exact native signatures remain in evidence/BackendSpec.
BackendWord GameUserData__Get(BackendRuntime&, const BackendWord* args, std::size_t argc);
BackendWord anonymous_namespace__EventDialogItem__EventDialogItem(BackendRuntime&, const BackendWord* args, std::size_t argc);
BackendWord Map__Get(BackendRuntime&, const BackendWord* args, std::size_t argc);
BackendWord GameSkip__IsBlackOut(BackendRuntime&, const BackendWord* args, std::size_t argc);
BackendWord GameSkip__IsSkip(BackendRuntime&, const BackendWord* args, std::size_t argc);
BackendWord ProcInst__Create(BackendRuntime&, const BackendWord* args, std::size_t argc);
BackendWord VariableManager__Set(BackendRuntime&, const BackendWord* args, std::size_t argc);
BackendWord BasicMenu__Create(BackendRuntime&, const BackendWord* args, std::size_t argc);
BackendWord BasicMenu__SetText(BackendRuntime&, const BackendWord* args, std::size_t argc);
BackendWord anonymous_namespace__EventDialog__EventDialog(BackendRuntime&, const BackendWord* args, std::size_t argc);
BackendWord FieldWorld__FindObject(BackendRuntime&, const BackendWord* args, std::size_t argc);
BackendWord Sound__IsEnableBGMCommand(BackendRuntime&, const BackendWord* args, std::size_t argc);
BackendWord Sound__EnableBGMCommand(BackendRuntime&, const BackendWord* args, std::size_t argc);
BackendWord Transporter__Get(BackendRuntime&, const BackendWord* args, std::size_t argc);
BackendWord GameMessage__CreateBind(BackendRuntime&, const BackendWord* args, std::size_t argc);
BackendWord Sound__SetEnableBGMCommand(BackendRuntime&, const BackendWord* args, std::size_t argc);
BackendWord ChapterSequence__GetInstance(BackendRuntime&, const BackendWord* args, std::size_t argc);
BackendWord Fade__IsActive(BackendRuntime&, const BackendWord* args, std::size_t argc);
BackendWord FieldWorld__GetHeightMap(BackendRuntime&, const BackendWord* args, std::size_t argc);
BackendWord GameMessage__SetGameSkipDisable(BackendRuntime&, const BackendWord* args, std::size_t argc);
BackendWord GameHandle__GameHandle_2(BackendRuntime&, const BackendWord* args, std::size_t argc);
BackendWord ProcGameInfo__SetUnit(BackendRuntime&, const BackendWord* args, std::size_t argc);
BackendWord Transporter__Add(BackendRuntime&, const BackendWord* args, std::size_t argc);
BackendWord HeightMap__GetMapHeight(BackendRuntime&, const BackendWord* args, std::size_t argc);
BackendWord Castle__Get(BackendRuntime&, const BackendWord* args, std::size_t argc);
BackendWord FieldWorld__GetScene(BackendRuntime&, const BackendWord* args, std::size_t argc);
BackendWord map__Draw__GetInstance(BackendRuntime&, const BackendWord* args, std::size_t argc);
BackendWord FieldWorld__GetParam(BackendRuntime&, const BackendWord* args, std::size_t argc);
BackendWord FlagNameManager__EntryGlobal(BackendRuntime&, const BackendWord* args, std::size_t argc);
BackendWord FlagNameManager__Entry(BackendRuntime&, const BackendWord* args, std::size_t argc);
BackendWord FlagManager__Set(BackendRuntime&, const BackendWord* args, std::size_t argc);
BackendWord ChapterSequence__GetStatus(BackendRuntime&, const BackendWord* args, std::size_t argc);
BackendWord VariableManager__Set_2(BackendRuntime&, const BackendWord* args, std::size_t argc);
BackendWord VariableManager__Add(BackendRuntime&, const BackendWord* args, std::size_t argc);
BackendWord Random__GetValue_2(BackendRuntime&, const BackendWord* args, std::size_t argc);
BackendWord Fade__FadeIn(BackendRuntime&, const BackendWord* args, std::size_t argc);
BackendWord Fade__FadeOut(BackendRuntime&, const BackendWord* args, std::size_t argc);
BackendWord ProcTalkManager__CreateInstanceBind(BackendRuntime&, const BackendWord* args, std::size_t argc);
BackendWord ProcTalkManager__Initialize(BackendRuntime&, const BackendWord* args, std::size_t argc);
BackendWord ProcInst__ProcInst(BackendRuntime&, const BackendWord* args, std::size_t argc);
BackendWord Spot__Get(BackendRuntime&, const BackendWord* args, std::size_t argc);
BackendWord Spot__GetData(BackendRuntime&, const BackendWord* args, std::size_t argc);
BackendWord Map__Load(BackendRuntime&, const BackendWord* args, std::size_t argc);
BackendWord ItemSkill__Get(BackendRuntime&, const BackendWord* args, std::size_t argc);
BackendWord ProcGameInfo__GetUnit(BackendRuntime&, const BackendWord* args, std::size_t argc);
BackendWord map__sound__Se__DangerOn(BackendRuntime&, const BackendWord* args, std::size_t argc);
BackendWord map__sound__Se__DangerOff(BackendRuntime&, const BackendWord* args, std::size_t argc);
BackendWord Transporter__Delete(BackendRuntime&, const BackendWord* args, std::size_t argc);
BackendWord FieldWorld__UpdateRange_2(BackendRuntime&, const BackendWord* args, std::size_t argc);
BackendWord GameFont__AddItem(BackendRuntime&, const BackendWord* args, std::size_t argc);
BackendWord map__sound__Se__ItemGain(BackendRuntime&, const BackendWord* args, std::size_t argc);
BackendWord castle__Accessory__AddAccessory(BackendRuntime&, const BackendWord* args, std::size_t argc);
BackendWord Movie__GetProc(BackendRuntime&, const BackendWord* args, std::size_t argc);
BackendWord Sound__BGMStop(BackendRuntime&, const BackendWord* args, std::size_t argc);
BackendWord FieldObject__SetState(BackendRuntime&, const BackendWord* args, std::size_t argc);
BackendWord GameLinkData__Dump(BackendRuntime&, const BackendWord* args, std::size_t argc);
BackendWord GameConfigData__IsSpeedFast(BackendRuntime&, const BackendWord* args, std::size_t argc);
BackendWord FlagManager__Clr(BackendRuntime&, const BackendWord* args, std::size_t argc);
BackendWord GameSkip__Trigger(BackendRuntime&, const BackendWord* args, std::size_t argc);
BackendWord GameSkipSequenceHelper__EscapeSkip(BackendRuntime&, const BackendWord* args, std::size_t argc);
BackendWord GameSkipSequenceHelper__EnableSkip(BackendRuntime&, const BackendWord* args, std::size_t argc);
BackendWord GameSkipSequenceHelper__DisableSkip(BackendRuntime&, const BackendWord* args, std::size_t argc);
BackendWord GameSkip__IsDisable(BackendRuntime&, const BackendWord* args, std::size_t argc);
BackendWord GameDialog__GameDialog(BackendRuntime&, const BackendWord* args, std::size_t argc);
BackendWord VariableManager__Get(BackendRuntime&, const BackendWord* args, std::size_t argc);
BackendWord ContentsUtil__IsOwnedRoute(BackendRuntime&, const BackendWord* args, std::size_t argc);
BackendWord BasicMenu__SetSelectFromIndexAsPossible(BackendRuntime&, const BackendWord* args, std::size_t argc);
BackendWord ProcInst__Create_2(BackendRuntime&, const BackendWord* args, std::size_t argc);
BackendWord ProcInst__Delete(BackendRuntime&, const BackendWord* args, std::size_t argc);
BackendWord castle__CastleWorld__Initialize(BackendRuntime&, const BackendWord* args, std::size_t argc);
BackendWord Map__Free(BackendRuntime&, const BackendWord* args, std::size_t argc);
BackendWord castle__CastleWorld__Finalize(BackendRuntime&, const BackendWord* args, std::size_t argc);
BackendWord map__Draw__Create(BackendRuntime&, const BackendWord* args, std::size_t argc);
BackendWord map__Draw__Delete(BackendRuntime&, const BackendWord* args, std::size_t argc);
BackendWord Proc__ResDelayBind(BackendRuntime&, const BackendWord* args, std::size_t argc);
BackendWord JobCategory__Get(BackendRuntime&, const BackendWord* args, std::size_t argc);
BackendWord map__ItemHelper__Rod__GetRescuePosition(BackendRuntime&, const BackendWord* args, std::size_t argc);
BackendWord unit__Enhance__AddWeakness(BackendRuntime&, const BackendWord* args, std::size_t argc);
BackendWord unit__Enhance__MergeWeakness(BackendRuntime&, const BackendWord* args, std::size_t argc);
BackendWord Fade__IsBlackOut(BackendRuntime&, const BackendWord* args, std::size_t argc);
BackendWord unit__AI__operator(BackendRuntime&, const BackendWord* args, std::size_t argc);
BackendWord UnitActor__SetMotion(BackendRuntime&, const BackendWord* args, std::size_t argc);
BackendWord UnitIcon__SetIcon_5(BackendRuntime&, const BackendWord* args, std::size_t argc);
BackendWord AIDesc__Get(BackendRuntime&, const BackendWord* args, std::size_t argc);
BackendWord AIValue__SetValue(BackendRuntime&, const BackendWord* args, std::size_t argc);
BackendWord Transporter__SetInvalidRefine(BackendRuntime&, const BackendWord* args, std::size_t argc);
BackendWord FieldWorld__UpdateRange_3(BackendRuntime&, const BackendWord* args, std::size_t argc);
BackendWord anonymous_namespace__GimmickMedicineImpl(BackendRuntime&, const BackendWord* args, std::size_t argc);
BackendWord Random__Initialize(BackendRuntime&, const BackendWord* args, std::size_t argc);
BackendWord map__Effect__Create_3(BackendRuntime&, const BackendWord* args, std::size_t argc);
BackendWord GameHandle__GameHandle(BackendRuntime&, const BackendWord* args, std::size_t argc);
BackendWord FlagManagerNoName__Set(BackendRuntime&, const BackendWord* args, std::size_t argc);
BackendWord map__NoticeWindow__Create(BackendRuntime&, const BackendWord* args, std::size_t argc);
BackendWord map__Intermediate__Tutorial__Create(BackendRuntime&, const BackendWord* args, std::size_t argc);
BackendWord FlagManagerNoName__SetAll(BackendRuntime&, const BackendWord* args, std::size_t argc);
BackendWord map__Draw__SuspendBind(BackendRuntime&, const BackendWord* args, std::size_t argc);
BackendWord map__Draw__ResumeBind(BackendRuntime&, const BackendWord* args, std::size_t argc);
BackendWord Movie__CreateBind(BackendRuntime&, const BackendWord* args, std::size_t argc);
BackendWord Movie__Create(BackendRuntime&, const BackendWord* args, std::size_t argc);
BackendWord Movie__IsPaused(BackendRuntime&, const BackendWord* args, std::size_t argc);
BackendWord Sound__BGMPlay(BackendRuntime&, const BackendWord* args, std::size_t argc);
BackendWord Sound__BGMTrackVolume(BackendRuntime&, const BackendWord* args, std::size_t argc);
BackendWord Sound__RBGMPlay(BackendRuntime&, const BackendWord* args, std::size_t argc);
BackendWord Sound__RBGMStop(BackendRuntime&, const BackendWord* args, std::size_t argc);
BackendWord Sound__RBGMEffect(BackendRuntime&, const BackendWord* args, std::size_t argc);
BackendWord Sound__BGMVolume(BackendRuntime&, const BackendWord* args, std::size_t argc);
BackendWord map__sound__Bgm__UpdateBGM(BackendRuntime&, const BackendWord* args, std::size_t argc);
BackendWord map__sound__Bgm__Map__Resume(BackendRuntime&, const BackendWord* args, std::size_t argc);
BackendWord Sound__SEPlay(BackendRuntime&, const BackendWord* args, std::size_t argc);
BackendWord HeightMap__GetHeight(BackendRuntime&, const BackendWord* args, std::size_t argc);
BackendWord Sound3D__Play(BackendRuntime&, const BackendWord* args, std::size_t argc);
BackendWord Sound__LSEPlay(BackendRuntime&, const BackendWord* args, std::size_t argc);
BackendWord Sound__Voice(BackendRuntime&, const BackendWord* args, std::size_t argc);
BackendWord Sound__EnvStop(BackendRuntime&, const BackendWord* args, std::size_t argc);
BackendWord Sound__EnvPlay(BackendRuntime&, const BackendWord* args, std::size_t argc);
BackendWord castle__SpotSelectSequence__MoveBind(BackendRuntime&, const BackendWord* args, std::size_t argc);
BackendWord FieldObject__SetVisible(BackendRuntime&, const BackendWord* args, std::size_t argc);
BackendWord FieldObject__SetEscape(BackendRuntime&, const BackendWord* args, std::size_t argc);
BackendWord FieldWorld__CreateObject(BackendRuntime&, const BackendWord* args, std::size_t argc);
BackendWord FieldObject__SetDispos(BackendRuntime&, const BackendWord* args, std::size_t argc);
BackendWord FieldWorld__DeleteObject(BackendRuntime&, const BackendWord* args, std::size_t argc);
BackendWord FieldObject__PlayAnime(BackendRuntime&, const BackendWord* args, std::size_t argc);
BackendWord FieldObject__GetLocatorIndex(BackendRuntime&, const BackendWord* args, std::size_t argc);
BackendWord FieldObject__GetLocatorPos_2(BackendRuntime&, const BackendWord* args, std::size_t argc);
BackendWord GameLinkGlobalData__Dump(BackendRuntime&, const BackendWord* args, std::size_t argc);
BackendWord DeliveryMessage__SetMessage(BackendRuntime&, const BackendWord* args, std::size_t argc);
BackendWord DeliveryMessage__Reset(BackendRuntime&, const BackendWord* args, std::size_t argc);
BackendWord ContentsLocal__GetElapse(BackendRuntime&, const BackendWord* args, std::size_t argc);
BackendWord ContentsLocal__UpdateTime(BackendRuntime&, const BackendWord* args, std::size_t argc);
BackendWord ContentsLocal__Dump(BackendRuntime&, const BackendWord* args, std::size_t argc);
BackendWord castle__AmiiboSequence__UpdateBind(BackendRuntime&, const BackendWord* args, std::size_t argc);

} // namespace fates::event::backend
