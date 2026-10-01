#include "fates/game/equip_skill_pool.hpp"
#include "fates/game/equip_skill.hpp"
#include "fates/detail/unit_ownership_runtime.hpp"
#include <algorithm>

namespace unit {
void EquipSkillPool::Clr(int skillIndex) {
    const int commonCount=EquipSkill::GetCommonNum();
    if(skillIndex<0 || skillIndex>=commonCount) return;
    bits_[static_cast<std::size_t>(skillIndex>>3)] &= static_cast<std::uint8_t>(~(1u<<(skillIndex&7)));
}
void EquipSkillPool::Set(int skillIndex) {
    const int commonCount=EquipSkill::GetCommonNum();
    if(skillIndex<0 || skillIndex>=commonCount) return;
    bits_[static_cast<std::size_t>(skillIndex>>3)] |= static_cast<std::uint8_t>(1u<<(skillIndex&7));
}
bool EquipSkillPool::Tst(int skillIndex) const {
    const int commonCount=EquipSkill::GetCommonNum();
    if(skillIndex<0 || skillIndex>=commonCount) return false;
    return (bits_[static_cast<std::size_t>(skillIndex>>3)] & static_cast<std::uint8_t>(1u<<(skillIndex&7))) != 0;
}
void EquipSkillPool::Merge(const EquipSkillPool* other) {
    if(other==nullptr) return;
    for(std::size_t i=0;i<bits_.size();++i) bits_[i]|=other->bits_[i];
}
void EquipSkillPool::Reset(){ bits_.fill(0); }
EquipSkillPool& EquipSkillPool::operator=(const EquipSkillPool& other){ bits_=other.bits_; return *this; }
void EquipSkillPool::Serialize(Stream* stream) const {
    // PROVEN wire format: version byte 1, length 0x14, then exactly 20 data bytes.
    fates::decomp_detail::UnitOwnershipCall("EquipSkillPool.Serialize.v1_20bytes", stream, bits_.data(), bits_.size());
}
void EquipSkillPool::Deserialize(Stream* stream) {
    // PROVEN: retail accepts the serialized length, copies the available bytes,
    // and zero-fills the remainder of the 20-byte pool for older data.
    fates::decomp_detail::UnitOwnershipCall("EquipSkillPool.Deserialize.versioned_20bytes", stream, bits_.data(), bits_.size());
}
}
