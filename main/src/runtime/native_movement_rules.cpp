#include "fates/runtime/native_movement_rules.hpp"
#include "fates/runtime/native_current_item_eligibility.hpp"
#include "fates/runtime/native_item_inventory.hpp"
#include "fates/battle/native_battle_conditions.hpp"
namespace fates::runtime::native {
namespace {
std::uint64_t Flags(const std::array<std::uint8_t,8>& bytes) noexcept {
    std::uint64_t result=0;for(unsigned i=0;i<8;++i)result|=std::uint64_t(bytes[i])<<(8*i);return result;
}
std::optional<std::uint64_t> CurrentPrivateFlags(const NativeRuntime& r,const UnitState& u) {
    const auto* p=r.definitions.FindPerson(u.person_id);const auto* j=r.definitions.FindJob(u.job_id);
    if(!u.occupied||!p||!j)return std::nullopt;
    return Flags(u.private_skill_bits)|Flags(p->bitflags)|Flags(j->bitflags);
}
}
std::optional<bool> HasHeldEnhancementExact(std::uint64_t flags,std::uint32_t public_flags,
    std::uint64_t item,bool special,const MovementRuleDefinitions& rules,HeldEnhancementServices& svc) {
    const auto allowed=rules.held_enhance|(special?(rules.fujin|rules.brynhildr):0);
    if(!(item&allowed))return false;
    for(unsigned i=0;i<6;++i)if((item&rules.item_restrictions[i])&&(flags&rules.person_restrictions[i])) {
        if(!(public_flags&0x10000000u))return true;
        const auto download=svc.PersonIsDownload();if(!download)return std::nullopt;
        if(*download)return true;
    }
    const auto group=svc.ItemWeaponGroup();if(!group)return std::nullopt;
    if(*group==8)return true;
    return svc.CanEquipWithStaffAndCurrentExp();
}
std::optional<bool> IsMovementCostFreeExact(std::uint16_t category,
    const MovementRuleDefinitions& rules,CostFreeServices& svc) {
    if(category&rules.shooter_category) {
        const auto skill=svc.HasAmphibiousSkill();if(!skill)return std::nullopt;
        if(*skill)return true;
    }
    for(unsigned i=0;i<5;++i) {
        const auto flags=svc.InventoryItemFlags(i);if(!flags)return std::nullopt;
        if(*flags&rules.fujin) {
            const auto enhance=svc.HasSpecialHeldEnhancement(i);if(!enhance)return std::nullopt;
            if(*enhance)return true;
        }
    }return false;
}
int TerrainCostFromUnitExact(std::int8_t cost,std::int8_t base,std::uint64_t flags,std::uint64_t mask) noexcept {
    return cost<0&&(flags&mask)?base:cost;
}
std::optional<bool> ProjectCurrentHeldEnhancement(const NativeRuntime& r,const UnitState& u,
    const ItemDefinition& item,bool special) {
    const auto flags=CurrentPrivateFlags(r,u);const auto& rules=r.definitions.movement_rules();
    if(!flags||!rules)return std::nullopt;
    struct Services:HeldEnhancementServices {
        const NativeRuntime& r;const UnitState& u;const ItemDefinition& item;
        Services(const NativeRuntime& r,const UnitState& u,const ItemDefinition& i):r(r),u(u),item(i){}
        std::optional<bool> PersonIsDownload() override {
            const auto* p=r.definitions.FindPerson(u.person_id);return p?r.definitions.IsPersonDownload(*p):std::nullopt;
        }
        std::optional<std::int8_t> ItemWeaponGroup() override {
            const auto* s=r.definitions.FindItemSubKind(item.weapon_category);
            return s?std::optional<std::int8_t>(std::int8_t(s->weapon_exp_group)):std::nullopt;
        }
        std::optional<bool> CanEquipWithStaffAndCurrentExp() override {
            switch(ProjectCurrentItemEligibility(r,u,item,true,true)) {
            case CurrentItemEligibility::Yes:return true;
            case CurrentItemEligibility::No:return false;
            default:return std::nullopt;
            }
        }
    } svc(r,u,item);
    return HasHeldEnhancementExact(*flags,u.flags,Flags(item.bitflags),special,*rules,svc);
}
std::optional<bool> ProjectCurrentMovementCostFree(const NativeRuntime& r,const UnitState& u) {
    const auto& rules=r.definitions.movement_rules();if(!rules||!CurrentPrivateFlags(r,u))return std::nullopt;
    const auto category=fates::battle::native::ProjectUnitBattleCategory(r,u);if(!category)return std::nullopt;
    struct Services:CostFreeServices {
        const NativeRuntime& r;const UnitState& u;
        Services(const NativeRuntime& r,const UnitState& u):r(r),u(u){}
        std::optional<bool> HasAmphibiousSkill() override {
            const auto* skill=r.definitions.FindSkill("SEID_\x90\x85\x97\xa4\x97\xbc\x97\x70");
            return skill?ProjectCurrentEquippedSkill(r,u,std::int16_t(skill->id)):std::nullopt;
        }
        const ItemDefinition* Item(unsigned index) {
            const auto inventory=ReadCalculationInventory(u);
            return inventory?r.definitions.FindItem((*inventory)[index].item_id):nullptr;
        }
        std::optional<std::uint64_t> InventoryItemFlags(unsigned index) override {
            const auto* item=Item(index);return item?std::optional<std::uint64_t>(Flags(item->bitflags)):std::nullopt;
        }
        std::optional<bool> HasSpecialHeldEnhancement(unsigned index) override {
            const auto* item=Item(index);return item?ProjectCurrentHeldEnhancement(r,u,*item,true):std::nullopt;
        }
    } svc(r,u);
    return IsMovementCostFreeExact(*category,*rules,svc);
}
std::optional<int> ProjectCurrentTerrainCost(const NativeRuntime& r,const UnitState& u,std::uint8_t index) {
    const auto flags=CurrentPrivateFlags(r,u);const auto& rules=r.definitions.movement_rules();
    if(!flags||!rules)return std::nullopt;
    const auto* job=r.definitions.FindJob(u.job_id);const auto& rows=r.definitions.movement_costs();
    if(job->movement_cost_index>=rows.size()||index>=rows[job->movement_cost_index].size())return std::nullopt;
    const auto cost=rows[job->movement_cost_index][index];
    if(cost>=0||!(*flags&rules->personal_terrain_cost))return int(cost);
    if(rows.empty()||index>=rows[0].size())return std::nullopt;
    return TerrainCostFromUnitExact(cost,rows[0][index],*flags,rules->personal_terrain_cost);
}
std::optional<bool> ProjectCurrentMovementProhibition(const NativeRuntime& r,const UnitState& u) {
    const auto flags=CurrentPrivateFlags(r,u);const auto& rules=r.definitions.movement_rules();
    if(!flags||!rules)return std::nullopt;
    return (*flags&rules->movement_prohibited)!=0;
}
std::optional<CurrentMovementRules> ProjectCurrentMovementRules(const NativeRuntime& r,const UnitState& u) {
    const auto flags=CurrentPrivateFlags(r,u);const auto& rules=r.definitions.movement_rules();
    if(!flags||!rules)return std::nullopt;
    const auto free=ProjectCurrentMovementCostFree(r,u);if(!free)return std::nullopt;
    return CurrentMovementRules{r.definitions.FindJob(u.job_id)->movement_cost_index,
        (*flags&rules->movement_prohibited)!=0,(*flags&rules->personal_terrain_cost)!=0,*free};
}
std::optional<bool> ProjectCurrentMovementBaseCostFallback(const NativeRuntime& r,const UnitState& u) {
    const auto flags=CurrentPrivateFlags(r,u);const auto& rules=r.definitions.movement_rules();
    if(!flags||!rules)return std::nullopt;return (*flags&rules->personal_terrain_cost)!=0;
}
}
