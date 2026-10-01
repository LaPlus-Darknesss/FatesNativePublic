#include "fates/battle/battle_unit.hpp"
#include "fates/detail/battle_runtime.hpp"

void BattleUnit::DeleteHair(){fates::decomp_detail::BattleUnitDeleteHair(*this);}
void BattleUnit::DeleteHead(){fates::decomp_detail::BattleUnitDeleteHead(*this);}
void BattleUnit::SetVisible(int a0, bool a1){fates::decomp_detail::BattleUnitSetVisible(*this, a0, a1);}
void BattleUnit::ConfrontDir(){fates::decomp_detail::BattleUnitConfrontDir(*this);}
void BattleUnit::CreateMagic(const char* a0, int a1){fates::decomp_detail::BattleUnitCreateMagic(*this, a0, a1);}
void BattleUnit::ParticleMan(const char* a0){fates::decomp_detail::BattleUnitParticleMan(*this, a0);}
void BattleUnit::ResetOnSkip(){fates::decomp_detail::BattleUnitResetOnSkip(*this);}
void BattleUnit::WaitLoading(){fates::decomp_detail::BattleUnitWaitLoading(*this);}
void BattleUnit::CreateEffect(const char* a0, const nn::math::MTX34* a1, int a2){fates::decomp_detail::BattleUnitCreateEffect(*this, a0, a1, a2);}
void BattleUnit::PlayFootStep(const char* a0){fates::decomp_detail::BattleUnitPlayFootStep(*this, a0);}
void BattleUnit::PlayWinVoice(const BattlePhase* a0){fates::decomp_detail::BattleUnitPlayWinVoice(*this, a0);}
void BattleUnit::SetTargetDir(const nn::math::VEC3& a0){fates::decomp_detail::BattleUnitSetTargetDir(*this, a0);}
void BattleUnit::SetTimeSpace(TimeSpace::Type a0){fates::decomp_detail::BattleUnitSetTimeSpace(*this, a0);}
void BattleUnit::UpdateTarget(const nn::math::VEC3& a0){fates::decomp_detail::BattleUnitUpdateTarget(*this, a0);}
const nn::math::MTX34* BattleUnit::FindBoneMtxPtr(const char* a0){return fates::decomp_detail::BattleUnitFindBoneMtxPtr(*this, a0);}
void BattleUnit::PlayDeathVoice(){fates::decomp_detail::BattleUnitPlayDeathVoice(*this);}
void BattleUnit::PlayMaterialSE(const char* a0){fates::decomp_detail::BattleUnitPlayMaterialSE(*this, a0);}
void BattleUnit::PlayThankVoice(){fates::decomp_detail::BattleUnitPlayThankVoice(*this);}
void BattleUnit::SetMagicMissed(){fates::decomp_detail::BattleUnitSetMagicMissed(*this);}
void BattleUnit::Tick_PreUpdate(){fates::decomp_detail::BattleUnitTick_PreUpdate(*this);}
void BattleUnit::PlayDamageVoice(const BattlePhase* a0){fates::decomp_detail::BattleUnitPlayDamageVoice(*this, a0);}
void BattleUnit::Tick_PostUpdate(){fates::decomp_detail::BattleUnitTick_PostUpdate(*this);}
void BattleUnit::CreateColorFader(int a0, int a1, int a2, int a3, int a4){fates::decomp_detail::BattleUnitCreateColorFader5(*this, a0, a1, a2, a3, a4);}
void BattleUnit::CreateColorFader(int a0, int a1, int a2, int a3, int a4, int a5){fates::decomp_detail::BattleUnitCreateColorFader6(*this, a0, a1, a2, a3, a4, a5);}
void BattleUnit::SetLocalTimeRate(float a0){fates::decomp_detail::BattleUnitSetLocalTimeRate(*this, a0);}
void BattleUnit::CreateChantEffect(){fates::decomp_detail::BattleUnitCreateChantEffect(*this);}
void BattleUnit::ExitSegmentRepeat(bool a0){fates::decomp_detail::BattleUnitExitSegmentRepeat(*this, a0);}
AnimObj* BattleUnit::GetPrimaryAnimObj(){return fates::decomp_detail::BattleUnitGetPrimaryAnimObj(*this);}
void BattleUnit::CreateShutterFader(float a0, float a1, float a2){fates::decomp_detail::BattleUnitCreateShutterFader(*this, a0, a1, a2);}
void BattleUnit::CreateStealthFader(float a0, float a1, float a2){fates::decomp_detail::BattleUnitCreateStealthFader(*this, a0, a1, a2);}
void BattleUnit::SetInitialLocation(const nn::math::VEC3& a0, const nn::math::VEC3& a1){fates::decomp_detail::BattleUnitSetInitialLocation(*this, a0, a1);}
void BattleUnit::SetMaterialEmotion(int a0){fates::decomp_detail::BattleUnitSetMaterialEmotion(*this, a0);}
void BattleUnit::SetOpMatVisibility(bool a0){fates::decomp_detail::BattleUnitSetOpMatVisibility(*this, a0);}
void BattleUnit::SetVisibleFromView(int a0, bool a1){fates::decomp_detail::BattleUnitSetVisibleFromView(*this, a0, a1);}
void BattleUnit::Tick_SignalExecute(){fates::decomp_detail::BattleUnitTick_SignalExecute(*this);}
void BattleUnit::AttachShopAccessory(int a0){fates::decomp_detail::BattleUnitAttachShopAccessory(*this, a0);}
void BattleUnit::DeleteBodyColliders(){fates::decomp_detail::BattleUnitDeleteBodyColliders(*this);}
void BattleUnit::DetachShopAccessory(int a0, int a1){fates::decomp_detail::BattleUnitDetachShopAccessory(*this, a0, a1);}
void BattleUnit::SetInitialDirection(){fates::decomp_detail::BattleUnitSetInitialDirection(*this);}
void BattleUnit::SetMaterialAddColor(const nw::ut::Color8& a0){fates::decomp_detail::BattleUnitSetMaterialAddColor(*this, a0);}
void BattleUnit::SetWeaponVisibility(bool a0){fates::decomp_detail::BattleUnitSetWeaponVisibility(*this, a0);}
void BattleUnit::UpdateTargetToEnemy(){fates::decomp_detail::BattleUnitUpdateTargetToEnemy(*this);}
void BattleUnit::CreateFootStepEffect(const char* a0){fates::decomp_detail::BattleUnitCreateFootStepEffect(*this, a0);}
void BattleUnit::SetMaterialHairColor(const nw::ut::Color8& a0){fates::decomp_detail::BattleUnitSetMaterialHairColor(*this, a0);}
void BattleUnit::SetMaterialMultColor(const Color8& a0){fates::decomp_detail::BattleUnitSetMaterialMultColor(*this, a0);}
void BattleUnit::SetMaterialSkinColor(const nw::ut::Color8& a0){fates::decomp_detail::BattleUnitSetMaterialSkinColor(*this, a0);}
void BattleUnit::ChangeMagicTargetSide(int a0){fates::decomp_detail::BattleUnitChangeMagicTargetSide(*this, a0);}
void BattleUnit::CreateClothSimulation(){fates::decomp_detail::BattleUnitCreateClothSimulation(*this);}
void BattleUnit::DeleteClothSimulation(){fates::decomp_detail::BattleUnitDeleteClothSimulation(*this);}
AnimObj* BattleUnit::GetPrimaryRideAnimObj(){return fates::decomp_detail::BattleUnitGetPrimaryRideAnimObj(*this);}
void BattleUnit::ReplaceBodyToSwimwear(){fates::decomp_detail::BattleUnitReplaceBodyToSwimwear(*this);}
void BattleUnit::SetMaterialClothDamage(int a0){fates::decomp_detail::BattleUnitSetMaterialClothDamage(*this, a0);}
void BattleUnit::SetMaterialEyePosition(const nn::math::VEC2& a0){fates::decomp_detail::BattleUnitSetMaterialEyePosition(*this, a0);}
void BattleUnit::CalcRootAnimDisplacement(float& a0, float& a1, float& a2){fates::decomp_detail::BattleUnitCalcRootAnimDisplacement(*this, a0, a1, a2);}
void BattleUnit::CreateTemplateColorFader(int a0){fates::decomp_detail::BattleUnitCreateTemplateColorFader(*this, a0);}
void BattleUnit::CreateFootStepEffectHuman(const char* a0){fates::decomp_detail::BattleUnitCreateFootStepEffectHuman(*this, a0);}
void BattleUnit::CreateUserDataParticleMan(){fates::decomp_detail::BattleUnitCreateUserDataParticleMan(*this);}
void BattleUnit::PlayAnimForceDirectEndPose(AnimClip::Type a0){fates::decomp_detail::BattleUnitPlayAnimForceDirectEndPose(*this, a0);}
void BattleUnit::ChangeToBeast(){fates::decomp_detail::BattleUnitChangeToBeast(*this);}
void BattleUnit::Setup(){fates::decomp_detail::BattleUnitSetup(*this);}
void BattleUnit::SetName(const char* a0){fates::decomp_detail::BattleUnitSetName(*this, a0);}
void BattleUnit::PlayAnim(const char* a0){fates::decomp_detail::BattleUnitPlayAnimName(*this, a0);}
void BattleUnit::PlayAnim(AnimClip::Type a0){fates::decomp_detail::BattleUnitPlayAnimClip(*this, a0);}
void BattleUnit::Retarget(){fates::decomp_detail::BattleUnitRetarget(*this);}
void BattleUnit::LoadAsync(BattleUnitResName& a0){fates::decomp_detail::BattleUnitLoadAsync(*this, a0);}
void BattleUnit::MakeDirty(const BattlePhase* a0){fates::decomp_detail::BattleUnitMakeDirty(*this, a0);}
void BattleUnit::PlayVoice(const char* a0){fates::decomp_detail::BattleUnitPlayVoice(*this, a0);}
void BattleUnit::Stabilize(){fates::decomp_detail::BattleUnitStabilize(*this);}
void BattleUnit::SyncWaste(){fates::decomp_detail::BattleUnitSyncWaste(*this);}
void BattleUnit::UpdateDir(){fates::decomp_detail::BattleUnitUpdateDir(*this);}
void BattleUnit::UpdatePos(){fates::decomp_detail::BattleUnitUpdatePos(*this);}
BattleUnit::BattleUnit(SceneSystem* a0, int a1){fates::decomp_detail::BattleUnitBattleUnit(*this, a0, a1);}
BattleUnit::~BattleUnit(){fates::decomp_detail::BattleUnitBattleUnit(*this);}
const BattleUnitResName* BattleUnit::GetResName() const{return fates::decomp_detail::BattleUnitGetResName(*this);}
nn::math::VEC3 BattleUnit::GetFocusPos() const{return fates::decomp_detail::BattleUnitGetFocusPos(*this);}
float BattleUnit::GetTimeRate() const{return fates::decomp_detail::BattleUnitGetTimeRate(*this);}
AnimClip::Type BattleUnit::GetReadyAnim() const{return fates::decomp_detail::BattleUnitGetReadyAnim(*this);}
int BattleUnit::GetDeathLevel() const{return fates::decomp_detail::BattleUnitGetDeathLevel(*this);}
const BattleUnitParams* BattleUnit::GetUnitParams() const{return fates::decomp_detail::BattleUnitGetUnitParams(*this);}
bool BattleUnit::IsAnimFinished() const{return fates::decomp_detail::BattleUnitIsAnimFinished(*this);}
BattleUnit* BattleUnit::OpponentPartner() const{return fates::decomp_detail::BattleUnitOpponentPartner(*this);}
nn::math::VEC3 BattleUnit::CalcDualChildPos(VEC2XZ& a0, VEC2XZ& a1, const VEC2XZ& a2, const VEC2XZ& a3, float a4) const{return fates::decomp_detail::BattleUnitCalcDualChildPos(*this, a0, a1, a2, a3, a4);}
int BattleUnit::GetMaterialEmotion() const{return fates::decomp_detail::BattleUnitGetMaterialEmotion(*this);}
std::uint8_t BattleUnit::GetMaterialOpacity() const{return fates::decomp_detail::BattleUnitGetMaterialOpacity(*this);}
AnimClip::Type BattleUnit::GetBattleInitialAnim() const{return fates::decomp_detail::BattleUnitGetBattleInitialAnim(*this);}
nn::math::VEC3 BattleUnit::GetActiveHandSlashVec(nn::math::VEC3& a0, nn::math::VEC3& a1, bool a2) const{return fates::decomp_detail::BattleUnitGetActiveHandSlashVec(*this, a0, a1, a2);}
nn::math::VEC2 BattleUnit::GetMaterialEyePosition() const{return fates::decomp_detail::BattleUnitGetMaterialEyePosition(*this);}
bool BattleUnit::IsLeftSideOnScreenSpace() const{return fates::decomp_detail::BattleUnitIsLeftSideOnScreenSpace(*this);}
int BattleUnit::FindNamedActorAndBoneIndex(const char* a0, int& a1) const{return fates::decomp_detail::BattleUnitFindNamedActorAndBoneIndex(*this, a0, a1);}
const char* BattleUnit::GetName() const{return fates::decomp_detail::BattleUnitGetName(*this);}
bool BattleUnit::IsNamed(const char* a0) const{return fates::decomp_detail::BattleUnitIsNamed(*this, a0);}
BattleUnit* BattleUnit::Partner() const{return fates::decomp_detail::BattleUnitPartner(*this);}
AABB BattleUnit::CalcAABB() const{return fates::decomp_detail::BattleUnitCalcAABB(*this);}
BattleUnit* BattleUnit::Opponent() const{return fates::decomp_detail::BattleUnitOpponent(*this);}
void BattleUnit::AreaClamp(VEC2XZ& a0, float a1) const{fates::decomp_detail::BattleUnitAreaClampXZ(*this, a0, a1);}
void BattleUnit::AreaClamp(nn::math::VEC3& a0, float a1) const{fates::decomp_detail::BattleUnitAreaClampVec3(*this, a0, a1);}
bool BattleUnit::IsStealth() const{return fates::decomp_detail::BattleUnitIsStealth(*this);}
