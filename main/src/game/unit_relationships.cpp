#include "fates/game/unit.hpp"

#include "fates/detail/gameplay_data_runtime.hpp"



void Unit::RelianceUp(Unit* a0) {
    fates::decomp_detail::GameplayDataCall("Unit.RelianceUp", this, a0);
}

void Unit::SetReliance(const Reliance* a0, int a1) {
    fates::decomp_detail::GameplayDataCall("Unit.SetReliance", this, a0, a1);
}

void Unit::CreateReliance(const unsigned char* a0) {
    fates::decomp_detail::GameplayDataCall("Unit.CreateReliance", this, a0);
}

void Unit::DeleteReliance() {
    fates::decomp_detail::GameplayDataCall("Unit.DeleteReliance", this);
}

void Unit::SetRelianceLevel(Unit* a0, int a1) {
    fates::decomp_detail::GameplayDataCall("Unit.SetRelianceLevel", this, a0, a1);
}

void Unit::AddChapterReliance(const Unit* a0, int a1) {
    fates::decomp_detail::GameplayDataCall("Unit.AddChapterReliance", this, a0, a1);
}

void Unit::AddChapterReliance(Unit* a0, Unit* a1, int a2) {
    fates::decomp_detail::GameplayDataCall("Unit.AddChapterReliance", a0, a1, a2);
}

void Unit::CreateChapterReliance(const unsigned char* a0) {
    fates::decomp_detail::GameplayDataCall("Unit.CreateChapterReliance", this, a0);
}

void Unit::DeleteChapterReliance() {
    fates::decomp_detail::GameplayDataCall("Unit.DeleteChapterReliance", this);
}

bool Unit::ChapterRelianceRankSortComp(unsigned int a0, unsigned int a1, void*) {
    return a0 < a1;
}

bool Unit::IsReliance(const Unit* a0) const {
    return fates::decomp_detail::GameplayDataValue<bool>("Unit.IsReliance", this, a0);
}

std::uint8_t Unit::GetReliance(const Reliance* a0) const {
    return fates::decomp_detail::GameplayDataValue<std::uint8_t>("Unit.GetReliance", this, a0);
}

bool Unit::IsRelianceUp(const Unit* a0) const {
    return fates::decomp_detail::GameplayDataValue<bool>("Unit.IsRelianceUp", this, a0);
}

std::uint32_t Unit::GetMarrigePid() const {
    return fates::decomp_detail::GameplayDataValue<std::uint32_t>("Unit.GetMarrigePid", this);
}

bool Unit::CanDoubleTrade(const Unit* a0) const {
    return fates::decomp_detail::GameplayDataValue<bool>("Unit.CanDoubleTrade", this, a0);
}

int Unit::GetDualSupport(BattleCapability::Type a0, int a1) const {
    return fates::decomp_detail::GameplayDataValue<int>("Unit.GetDualSupport", this, a0, a1);
}

bool Unit::IsRelianceTalk(const Unit* a0, int a1) const {
    return fates::decomp_detail::GameplayDataValue<bool>("Unit.IsRelianceTalk", this, a0, a1);
}

const Person* Unit::GetMarrigePerson() const {
    return fates::decomp_detail::GameplayDataValue<const Person*>("Unit.GetMarrigePerson", this);
}

int Unit::GetRelianceLevel(const Unit* a0) const {
    return fates::decomp_detail::GameplayDataValue<int>("Unit.GetRelianceLevel", this, a0);
}

bool Unit::IsFamilyUncleAunt(const Unit* a0) const {
    return fates::decomp_detail::GameplayDataValue<bool>("Unit.IsFamilyUncleAunt", this, a0);
}

bool Unit::IsRelianceInvalid() const {
    return fates::decomp_detail::GameplayDataValue<bool>("Unit.IsRelianceInvalid", this);
}

std::uint8_t Unit::GetChapterReliance(const Unit* a0) const {
    return fates::decomp_detail::GameplayDataValue<std::uint8_t>("Unit.GetChapterReliance", this, a0);
}

Unit* Unit::GetFamilyChild(const Unit* a0) const {
    return fates::decomp_detail::GameplayDataValue<Unit*>("Unit.GetFamilyChild", this, a0);
}

Unit* Unit::GetFamilyChildImpl(const Unit* a0) const {
    return fates::decomp_detail::GameplayDataValue<Unit*>("Unit.GetFamilyChildImpl", this, a0);
}

int Unit::GetRelianceMaxLevel(const Unit* a0) const {
    return fates::decomp_detail::GameplayDataValue<int>("Unit.GetRelianceMaxLevel", this, a0);
}

bool Unit::IsFamilyParentChild(const Unit* a0) const {
    return fates::decomp_detail::GameplayDataValue<bool>("Unit.IsFamilyParentChild", this, a0);
}

int Unit::GetRelianceScoreImpl(bool* a0, int* a1, int* a2, int* a3, const Unit* a4) const {
    return fates::decomp_detail::GameplayDataValue<int>("Unit.GetRelianceScoreImpl", this, a0, a1, a2, a3, a4);
}

bool Unit::IsFamilyBrotherSister(const Unit* a0) const {
    return fates::decomp_detail::GameplayDataValue<bool>("Unit.IsFamilyBrotherSister", this, a0);
}

int Unit::GetChapterRelianceRank(const Unit* a0) const {
    return fates::decomp_detail::GameplayDataValue<int>("Unit.GetChapterRelianceRank", this, a0);
}

int Unit::GetRelianceScoreForDual(const Unit* a0) const {
    return fates::decomp_detail::GameplayDataValue<int>("Unit.GetRelianceScoreForDual", this, a0);
}

int Unit::GetRelianceScoreForUnitStatus(const Unit* a0) const {
    return fates::decomp_detail::GameplayDataValue<int>("Unit.GetRelianceScoreForUnitStatus", this, a0);
}

bool Unit::IsSortieForUnitStatusReliance() const {
    return fates::decomp_detail::GameplayDataValue<bool>("Unit.IsSortieForUnitStatusReliance", this);
}

bool Unit::IsRelianceInvalidForClassChange() const {
    return fates::decomp_detail::GameplayDataValue<bool>("Unit.IsRelianceInvalidForClassChange", this);
}

bool Unit::IsFamily(const Unit* a0) const {
    return fates::decomp_detail::GameplayDataValue<bool>("Unit.IsFamily", this, a0);
}

bool Unit::IsMarrige() const {
    // Retail spelling retained. The helper is exactly the existence test for
    // the marriage/S-rank partner identifier.
    return GetMarrigePid() != 0;
}

bool Unit::IsRomance(const Unit* a0) const {
    return fates::decomp_detail::GameplayDataValue<bool>("Unit.IsRomance", this, a0);
}
