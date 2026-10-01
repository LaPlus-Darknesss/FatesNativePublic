#include "fates/battle/action_states.hpp"
#include "fates/detail/battle_action_runtime.hpp"

void BattleArgs::Initialize() { fates::decomp_detail::BattleActionCall("BattleArgs::Initialize"); }
void BattleArgs::PostProcess() { fates::decomp_detail::BattleActionCall("BattleArgs::PostProcess", *this); }
void BattleArgs::BuildActionState(BattleUnit* a0, BattleUnit* a1) { fates::decomp_detail::BattleActionCall("BattleArgs::BuildActionState", *this, a0, a1); }
void BattleArgs::ClearBattlePhase() { fates::decomp_detail::BattleActionCall("BattleArgs::ClearBattlePhase", *this); }
void BattleArgs::CreateBattleUnits() { fates::decomp_detail::BattleActionCall("BattleArgs::CreateBattleUnits", *this); }
void BattleArgs::Sync(const BattlePhase* a0) { fates::decomp_detail::BattleActionCall("BattleArgs::Sync", *this, a0); }
void BattleArgs::Import(map::BattleCalculator* a0) { fates::decomp_detail::BattleActionCall("BattleArgs::Import", *this, a0); }
void BattleArgs::Import(int a0, const Unit* a1) { fates::decomp_detail::BattleActionCall("BattleArgs::Import", *this, a0, a1); }
void BattleArgs::Finalize() { fates::decomp_detail::BattleActionCall("BattleArgs::Finalize"); }
int BattleArgs::GetLastPhaseIndex() const { return fates::decomp_detail::BattleActionValue<int>("BattleArgs::GetLastPhaseIndex", *this); }
void BattlePhase::DecideDamageAuto() { fates::decomp_detail::BattleActionCall("BattlePhase::DecideDamageAuto", *this); }
int BattlePhase::GetDamageEmotion(AnimClip::Type a0) { const int clip=static_cast<int>(a0); return clip==0x0F ? 5 : (clip==0x10 ? 6 : 7); }
int BattlePhase::GetDamageEmotionTime(AnimClip::Type a0) { const int clip=static_cast<int>(a0); if(clip==0x0F) return 250; if(clip==0x10) return 333; if(clip==0x13) return -1; return 666; }
BattlePhase::BattlePhase() { fates::decomp_detail::BattleActionCall("BattlePhase::BattlePhase", *this); }
AnimClip::Type BattlePhase::GetAttackClip() const { return fates::decomp_detail::BattleActionValue<AnimClip::Type>("BattlePhase::GetAttackClip", *this); }
AnimClip::Type BattlePhase::GetDamageClip() const { return fates::decomp_detail::BattleActionValue<AnimClip::Type>("BattlePhase::GetDamageClip", *this); }
bool BattlePhase::IsEventBattle() const { return fates::decomp_detail::BattleActionValue<bool>("BattlePhase::IsEventBattle", *this); }
bool BattlePhase::IsSomeoneDead() const { return fates::decomp_detail::BattleActionValue<bool>("BattlePhase::IsSomeoneDead", *this); }
bool BattlePhase::NeedImpactGuard() const { return fates::decomp_detail::BattleActionValue<bool>("BattlePhase::NeedImpactGuard", *this); }
bool BattlePhase::NeedImpactShake() const { return fates::decomp_detail::BattleActionValue<bool>("BattlePhase::NeedImpactShake", *this); }
int BattlePhase::GetHitStopFrames() const { return fates::decomp_detail::BattleActionValue<int>("BattlePhase::GetHitStopFrames", *this); }
bool BattleSoundActor::IsVoiceCanceled() { return fates::decomp_detail::BattleActionValue<bool>("BattleSoundActor::IsVoiceCanceled", *this); }
void BattleSoundActor::SetSoundPosition(const nn::math::VEC3& a0) { fates::decomp_detail::BattleActionCall("BattleSoundActor::SetSoundPosition", *this, a0); }
void BattleSoundActor::PlaySound(const char* a0) { fates::decomp_detail::BattleActionCall("BattleSoundActor::PlaySound", *this, a0); }
BattleSoundActor::BattleSoundActor(int a0) { fates::decomp_detail::BattleActionCall("BattleSoundActor::BattleSoundActor", *this, a0); }
BattleSoundActor::~BattleSoundActor() { fates::decomp_detail::BattleActionCall("BattleSoundActor::~BattleSoundActor", *this); }
void BattleUnitParams::Import(const Unit* a0) { fates::decomp_detail::BattleActionCall("BattleUnitParams::Import", *this, a0); }
void BattleUnitParams::Import(const map::BattleInfo* a0, int a1) { fates::decomp_detail::BattleActionCall("BattleUnitParams::Import", *this, a0, a1); }
std::uint64_t BattleUnitParams::IsReverse(const map::BattleInfo* a0) { return fates::decomp_detail::BattleActionValue<std::uint64_t>("BattleUnitParams::IsReverse", *this, a0); }
BattleUnitParams::BattleUnitParams() { fates::decomp_detail::BattleActionCall("BattleUnitParams::BattleUnitParams", *this); }
const void* BattleUnitParams::GetWeaponData() const { return fates::decomp_detail::BattleActionValue<const void*>("BattleUnitParams::GetWeaponData", *this); }
void BattleUnitResName::ClearAnims(unsigned int a0) { fates::decomp_detail::BattleActionCall("BattleUnitResName::ClearAnims", *this, a0); }
void BattleUnitResName::PostProcess(const Unit* a0, const Person* a1) { fates::decomp_detail::BattleActionCall("BattleUnitResName::PostProcess", *this, a0, a1); }
void BattleUnitResName::AddAccessory(int a0, const char* a1, const char* a2) { fates::decomp_detail::BattleActionCall("BattleUnitResName::AddAccessory", *this, a0, a1, a2); }
void BattleUnitResName::ClearEventAnims() { fates::decomp_detail::BattleActionCall("BattleUnitResName::ClearEventAnims", *this); }
void BattleUnitResName::ConstructDirect(const char* a0, unsigned int a1, const char* a2) { fates::decomp_detail::BattleActionCall("BattleUnitResName::ConstructDirect", *this, a0, a1, a2); }
void BattleUnitResName::ClearBattleAnims() { fates::decomp_detail::BattleActionCall("BattleUnitResName::ClearBattleAnims", *this); }
void BattleUnitResName::ConstructHoodMan(bool a0) { fates::decomp_detail::BattleActionCall("BattleUnitResName::ConstructHoodMan", *this, a0); }
void BattleUnitResName::ModifySwimwearName(const Unit* a0, const Person* a1, const Job* a2) { fates::decomp_detail::BattleActionCall("BattleUnitResName::ModifySwimwearName", *this, a0, a1, a2); }
void BattleUnitResName::ClearExceptDeformAnims() { fates::decomp_detail::BattleActionCall("BattleUnitResName::ClearExceptDeformAnims", *this); }
void BattleUnitResName::ModifyForceTextureName(int a0, bool a1) { fates::decomp_detail::BattleActionCall("BattleUnitResName::ModifyForceTextureName", *this, a0, a1); }
void BattleUnitResName::ClearExceptSubjectAnims() { fates::decomp_detail::BattleActionCall("BattleUnitResName::ClearExceptSubjectAnims", *this); }
void BattleUnitResName::Import(const unsigned int* a0) { fates::decomp_detail::BattleActionCall("BattleUnitResName::Import", *this, a0); }
void BattleUnitResName::Import(int a0, unsigned int a1, unsigned int a2, const unsigned int* a3) { fates::decomp_detail::BattleActionCall("BattleUnitResName::Import", *this, a0, a1, a2, a3); }
void BattleUnitResName::Construct(const Unit* a0, const Person* a1, const char* a2, const Job* a3, const char* a4, unsigned int a5) { fates::decomp_detail::BattleActionCall("BattleUnitResName::Construct", *this, a0, a1, a2, a3, a4, a5); }
void BattleUnitResName::Construct(const Unit* a0, unsigned int a1, const char* a2) { fates::decomp_detail::BattleActionCall("BattleUnitResName::Construct", *this, a0, a1, a2); }
void BattleUnitResName::Construct(const char* a0, const Job* a1, unsigned int a2, const char* a3) { fates::decomp_detail::BattleActionCall("BattleUnitResName::Construct", *this, a0, a1, a2, a3); }
void BattleUnitResName::Construct(const char* a0, unsigned int a1, const char* a2) { fates::decomp_detail::BattleActionCall("BattleUnitResName::Construct", *this, a0, a1, a2); }
BattleUnitResName::BattleUnitResName() { fates::decomp_detail::BattleActionCall("BattleUnitResName::BattleUnitResName", *this); }
const void* BattleUnitResHolder::GetSignals(AnimClip::Type a0) { return fates::decomp_detail::BattleActionValue<const void*>("BattleUnitResHolder::GetSignals", *this, a0); }
void BattleUnitResHolder::PreloadModels(const BattleUnitResName& a0, const BattleUnit* a1) { fates::decomp_detail::BattleActionCall("BattleUnitResHolder::PreloadModels", *this, a0, a1); }
void BattleUnitResHolder::PreloadAnims(const BattleUnitResName& a0) { fates::decomp_detail::BattleActionCall("BattleUnitResHolder::PreloadAnims", *this, a0); }
void BattleUnitResHolder::loadanim(const BattleUnitResName& a0, AnimClip::Type a1, FilePrior::Level a2) { fates::decomp_detail::BattleActionCall("BattleUnitResHolder::loadanim", *this, a0, a1, a2); }
bool BattleUnitResHolder::IsLoading() { return fates::decomp_detail::BattleActionValue<bool>("BattleUnitResHolder::IsLoading", *this); }
BattleUnitResHolder::BattleUnitResHolder(BattleUnit* a0) { fates::decomp_detail::BattleActionCall("BattleUnitResHolder::BattleUnitResHolder", *this, a0); }
BattleUnitResHolder::~BattleUnitResHolder() { fates::decomp_detail::BattleActionCall("BattleUnitResHolder::~BattleUnitResHolder", *this); }
AnimClip::Type BattleUnitResHolder::GetSimilarAnim(AnimClip::Type a0) const { return fates::decomp_detail::BattleActionValue<AnimClip::Type>("BattleUnitResHolder::GetSimilarAnim", *this, a0); }
void BattleUnitColorFader::Persistent() { fates::decomp_detail::BattleActionCall("BattleUnitColorFader::Persistent", *this); }
void BattleUnitScratchFader::Effect() { fates::decomp_detail::BattleActionCall("BattleUnitScratchFader::Effect", *this); }
void BattleUnitStealthFader::Persistent() { fates::decomp_detail::BattleActionCall("BattleUnitStealthFader::Persistent", *this); }
void BattleUnitStealthFader::Effect() { fates::decomp_detail::BattleActionCall("BattleUnitStealthFader::Effect", *this); }
