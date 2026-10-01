#include "fates/support/native_family_support.hpp"
#include "fates/support/native_local_support.hpp"
namespace fates::support::native {
namespace rn=fates::runtime::native;
namespace {
const SupportParentState* Parent(const SupportFamilyState* family,std::uint16_t person) {
    if(family)for(const auto& p:family->parents)if(p.person && *p.person==person)return &p;
    return nullptr;
}
bool Complete(const SupportFamilyState* f) {return f && f->parents[0].person && f->parents[1].person;}
std::optional<std::uint16_t> Grand(const SupportFamilyState& f,bool mother) {
    auto a=mother?f.parents[0].mother:f.parents[0].father;
    return a?a:(mother?f.parents[1].mother:f.parents[1].father);
}
bool Edit(const rn::DefinitionStore& d,SupportEditState e,std::array<int,2>& out) {
    const auto* boon=d.FindPersonality(e.boon);const auto* bane=d.FindPersonality(e.bane);
    if(!boon || !bane || boon->attack_stance_capability < -1 || boon->attack_stance_capability>3 ||
       bane->attack_stance_capability < -1 || bane->attack_stance_capability>3)return false;
    out={boon->attack_stance_capability,bane->attack_stance_capability};return true;
}
}
int FamilySupportLevelExact(int points) noexcept {return points>=10?3:points>=5?2:points>=1?1:0;}
FamilySupportRelationship ResolveFamilySupportExact(bool eligible,std::uint16_t subject,std::uint16_t other,
    const SupportFamilyState* a,const SupportFamilyState* b) noexcept {
    if(!eligible)return {};
    if(const auto* p=Parent(a,other))return {FamilyRelationshipKind::ParentChild,1,p->points};
    if(const auto* p=Parent(b,subject))return {FamilyRelationshipKind::ParentChild,2,p->points};
    if(a && b)for(unsigned i=0;i<2;++i)if(a->parents[i].person && a->parents[i].person==b->parents[i].person)
        return {FamilyRelationshipKind::Siblings,0,a->sibling_points};
    return {};
}
bool IsUncleAuntSupportExact(std::uint16_t subject,std::uint16_t other,
    const SupportFamilyState* a,const SupportFamilyState* b) noexcept {
    if(Parent(a,other) || Parent(b,subject) || !Complete(a) || !Complete(b))return false;
    for(unsigned i=0;i<2;++i) {
        const auto ga=Grand(*a,i!=0),gb=Grand(*b,i!=0);
        if((ga && ga==b->parents[i].person) || (gb && gb==a->parents[i].person))return true;
    }
    return false;
}
int PersonEditSupportExact(const std::array<std::int8_t,20>& table,int cap,int level,int boon,int bane) noexcept {
    if(level<0 || level>4 || cap<0 || cap>3 || boon<0 || boon>3 || bane<0 || bane>3 || boon==bane || (cap!=boon && cap!=bane))return 0;
    int transferred=0;for(int row=1;row<=level;++row)transferred+=table[std::size_t(row*4+bane)];
    if(cap==bane)return -transferred;
    if(bane==1)return (transferred*5)/3;
    if(boon==1)return (transferred*3)/5;
    return transferred;
}
int FamilyAttackStanceTotalExact(const std::array<std::int8_t,20>& first,const std::array<std::int8_t,20>& second,
    int cap,int level,const std::array<int,2>& first_edit,const std::array<int,2>& second_edit) noexcept {
    if(cap<0 || cap>3 || level<0 || level>4)return 0;
    int total=first[std::size_t(cap)];
    for(int row=1;row<=level;++row) {
        const auto& table=(row&1)?second:first;const auto& edit=(row&1)?second_edit:first_edit;
        total+=table[std::size_t(row*4+cap)];
        total+=PersonEditSupportExact(table,cap,row,edit[0],edit[1])-PersonEditSupportExact(table,cap,row-1,edit[0],edit[1]);
    }
    return total;
}
CarriedBonusStatus ResolveCarriedSupportBonuses(const rn::DefinitionStore& d,std::uint16_t id,
    const SupportFamilyState* family,const SupportEditState* edit,std::array<std::array<std::int16_t,4>,5>& result) {
    const auto* own=d.FindPerson(id);if(!own)return CarriedBonusStatus::MissingPerson;
    const auto* fixed=own->parent?d.FindPerson(own->parent):nullptr;
    // Unloaded references cannot establish retail's resolved-null lookup.
    if(own->parent && !fixed)return CarriedBonusStatus::MissingPerson;
    const rn::PersonDefinition* first=fixed?fixed:own;const rn::PersonDefinition* second=nullptr;
    std::array<int,2> first_edit{-1,-1},second_edit{-1,-1};
    if(fixed && Complete(family)) {
        const rn::PersonDefinition* parents[2]{};std::array<int,2> edits[2]{{-1,-1},{-1,-1}};
        for(unsigned i=0;i<2;++i) {
            const auto& f=family->parents[i];parents[i]=d.FindPerson(*f.person);
            if(!parents[i])return CarriedBonusStatus::MissingPerson;
            const auto* parent_fixed=parents[i]->parent?d.FindPerson(parents[i]->parent):nullptr;
            if(parents[i]->parent && !parent_fixed)return CarriedBonusStatus::MissingPerson;
            const auto grand=i==0?f.father:f.mother;
            if(parent_fixed && grand) {
                parents[i]=d.FindPerson(*grand);if(!parents[i])return CarriedBonusStatus::MissingPerson;
            } else if(f.edit.boon && f.edit.bane && !Edit(d,f.edit,edits[i]))return CarriedBonusStatus::MissingPersonality;
        }
        first=parents[0];second=parents[1];first_edit=edits[0];second_edit=edits[1];
    } else if(!fixed && edit && !Edit(d,*edit,first_edit))return CarriedBonusStatus::MissingPersonality;
    if(!first->attack_stance_bonuses_present || (second && !second->attack_stance_bonuses_present))return CarriedBonusStatus::MissingTable;
    auto next=result;
    for(unsigned level=0;level<5;++level)for(unsigned cap=0;cap<4;++cap) {
        const int total=second?FamilyAttackStanceTotalExact(first->attack_stance_increments,second->attack_stance_increments,int(cap),int(level),first_edit,second_edit):
            PersonAttackStanceTotalExact(first->attack_stance_increments,std::uint8_t(cap),int(level))+
            PersonEditSupportExact(first->attack_stance_increments,int(cap),int(level),first_edit[0],first_edit[1]);
        next[level][cap]=std::int16_t(total);
    }
    result=next;return CarriedBonusStatus::Ok;
}
}
