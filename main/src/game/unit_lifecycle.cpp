#include "fates/game/unit.hpp"

#include "fates/detail/gameplay_data_runtime.hpp"



void Unit::AIActivate(bool a0) {
    fates::decomp_detail::GameplayDataCall("Unit.AIActivate", this, a0);
}

bool Unit::CreateImpl(const Person* a0, const Job* a1, int a2, Random* a3) {
    return fates::decomp_detail::GameplayDataValue<bool>("Unit.CreateImpl", this, a0, a1, a2, a3);
}

void Unit::Initialize(int a0) {
    fates::decomp_detail::GameplayDataCall("Unit.Initialize", this, a0);
}

void Unit::CreateEdit(const unit::Edit* a0) {
    fates::decomp_detail::GameplayDataCall("Unit.CreateEdit", this, a0);
}

void Unit::UpdateEdit() {
    fates::decomp_detail::GameplayDataCall("Unit.UpdateEdit", this);
}

void Unit::ClassChange(const Job* a0, int a1) {
    fates::decomp_detail::GameplayDataCall("Unit.ClassChange", this, a0, a1);
}

map::Actor* Unit::CreateActor() {
    return fates::decomp_detail::GameplayDataValue<map::Actor*>("Unit.CreateActor", this);
}

void Unit::CreateClone(Unit* a0) {
    fates::decomp_detail::GameplayDataCall("Unit.CreateClone", this, a0);
}

void Unit::CreateForCastleJoin(const Person* a0) {
    fates::decomp_detail::GameplayDataCall("Unit.CreateForCastleJoin", this, a0);
}

void Unit::CreateImpl2(bool a0, bool a1) {
    fates::decomp_detail::GameplayDataCall("Unit.CreateImpl2", this, a0, a1);
}

void Unit::Deserialize(Stream* a0) {
    fates::decomp_detail::GameplayDataCall("Unit.Deserialize", this, a0);
}

void Unit::DetachClone() {
    fates::decomp_detail::GameplayDataCall("Unit.DetachClone", this);
}

void Unit::UpdateActor() {
    fates::decomp_detail::GameplayDataCall("Unit.UpdateActor", this);
}

void Unit::UpdateClone() {
    fates::decomp_detail::GameplayDataCall("Unit.UpdateClone", this);
}

void Unit::TransferForSortie(Force::Type a0, bool a1) {
    fates::decomp_detail::GameplayDataCall("Unit.TransferForSortie", this, a0, a1);
}

void Unit::TransferImpl(Force::Type a0, bool a1, bool a2) {
    fates::decomp_detail::GameplayDataCall("Unit.TransferImpl", this, a0, a1, a2);
}

void Unit::CopyForBackup(const Unit* a0) {
    fates::decomp_detail::GameplayDataCall("Unit.CopyForBackup", this, a0);
}

void Unit::ResetEndOfMap(bool a0, bool a1) {
    fates::decomp_detail::GameplayDataCall("Unit.ResetEndOfMap", this, a0, a1);
}

void Unit::UpdateCloneHP() {
    fates::decomp_detail::GameplayDataCall("Unit.UpdateCloneHP", this);
}

void Unit::CopyForLevelUp(const Unit* a0) {
    fates::decomp_detail::GameplayDataCall("Unit.CopyForLevelUp", this, a0);
}

void Unit::CopyForRestore(Unit* a0, bool a1, bool a2) {
    fates::decomp_detail::GameplayDataCall("Unit.CopyForRestore", this, a0, a1, a2);
}

void Unit::SetPlayerBelong() {
    fates::decomp_detail::GameplayDataCall("Unit.SetPlayerBelong", this);
}

void Unit::UpdateCloneItem() {
    fates::decomp_detail::GameplayDataCall("Unit.UpdateCloneItem", this);
}

void Unit::UpdateHairColor() {
    fates::decomp_detail::GameplayDataCall("Unit.UpdateHairColor", this);
}

void Unit::CreateFromDispos(const void* a0, int a1) {
    fates::decomp_detail::GameplayDataCall("Unit.CreateFromDispos", this, a0, a1);
}

void Unit::CreateFromPacket(const game::packet::Unit* a0) {
    fates::decomp_detail::GameplayDataCall("Unit.CreateFromPacket", this, a0);
}

void Unit::AIActivate_CauseAttacked(bool a0) {
    fates::decomp_detail::GameplayDataCall("Unit.AIActivate_CauseAttacked", this, a0);
}

void Unit::CreateFromExtendedPacket(const game::packet::ExtendedUnit* a0, bool a1) {
    fates::decomp_detail::GameplayDataCall("Unit.CreateFromExtendedPacket", this, a0, a1);
}

void Unit::Copy(const Unit* a0) {
    fates::decomp_detail::GameplayDataCall("Unit.Copy", this, a0);
}

void Unit::Clear() {
    fates::decomp_detail::GameplayDataCall("Unit.Clear", this);
}

void Unit::Create(const Person* a0, const Job* a1, int a2, Random* a3) {
    fates::decomp_detail::GameplayDataCall("Unit.Create", this, a0, a1, a2, a3);
}

void Unit::LevelUp() {
    fates::decomp_detail::GameplayDataCall("Unit.LevelUp", this);
}

void Unit::DoubleOn(Unit* a0) {
    fates::decomp_detail::GameplayDataCall("Unit.DoubleOn", this, a0);
}

void Unit::Handover(bool a0) {
    fates::decomp_detail::GameplayDataCall("Unit.Handover", this, a0);
}

void Unit::Inherit2() {
    fates::decomp_detail::GameplayDataCall("Unit.Inherit2", this);
}

void Unit::Transfer(Force::Type a0, bool a1) {
    fates::decomp_detail::GameplayDataCall("Unit.Transfer", this, a0, a1);
}

void Unit::DoubleOff() {
    fates::decomp_detail::GameplayDataCall("Unit.DoubleOff", this);
}

void Unit::Reinstate(const void* a0) {
    fates::decomp_detail::GameplayDataCall("Unit.Reinstate", this, a0);
}

void Unit::SetDispos(const void* a0) {
    fates::decomp_detail::GameplayDataCall("Unit.SetDispos", this, a0);
}

Unit::Unit() {
    fates::decomp_detail::GameplayDataCall("Unit.Construct", this);
}

Unit::~Unit() {
    // Retail performs real cleanup of optional owned buffers/family state; the
    // member layout remains intentionally opaque until the ownership families land.
    fates::decomp_detail::GameplayDataCall("Unit.Destroy", this);
}

Unit& Unit::operator=(const Unit& a0) {
    fates::decomp_detail::GameplayDataCall("Unit.DeepCopyAssign", this, &a0);
    return *this;
}

void Unit::CreateFromClerk(const Person* a0, const Job* a1) {
    fates::decomp_detail::GameplayDataCall("Unit.CreateFromClerk", this, a0, a1);
}
