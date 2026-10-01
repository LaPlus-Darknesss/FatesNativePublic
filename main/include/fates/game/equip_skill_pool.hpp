#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
class Stream;

namespace unit {
class EquipSkillPool {
public:
    static constexpr std::size_t kSerializedBytes = 0x14;

    void Deserialize(Stream* stream);
    void Clr(int skillIndex);
    void Set(int skillIndex);
    void Merge(const EquipSkillPool* other);
    void Reset();
    EquipSkillPool& operator=(const EquipSkillPool& other);
    bool Tst(int skillIndex) const;
    void Serialize(Stream* stream) const;

private:
    // PROVEN: retail clears/merges/serializes exactly twenty bytes. Each common
    // EquipSkill index is one bit. This is safe to model concretely.
    std::array<std::uint8_t, kSerializedBytes> bits_{};
};
static_assert(sizeof(EquipSkillPool) == 0x14);
}
