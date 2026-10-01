#pragma once
#include <cstdint>
#include <optional>
#include <vector>

namespace fates::map::native {
struct UnitIconAnimationSnapshot {
    std::uint32_t flags{};
    std::uint16_t frame{};
    std::uint8_t animation{};
    bool operator==(const UnitIconAnimationSnapshot&) const=default;
};
struct ActorVisualSnapshot {
    std::uint8_t alpha{},motion{},countdown_2b{},countdown_2c{};
    std::uint16_t counter_2e{},counter_30{};
    std::uint32_t flags_38{};
    // Missing means the icon state has not been carried/bound, not a null
    // UnitActor icon that permits SetMotion to succeed without its writes.
    std::optional<UnitIconAnimationSnapshot> icon;
    bool operator==(const ActorVisualSnapshot&) const=default;
};
class NativeActorVisualState {
    friend struct ActorVisualAccess;
    struct Record {
        std::uint16_t slot{},person{};
        std::uint64_t generation{},transfer_revision{};
        std::uint8_t chapter{};
        bool map_active{};
        ActorVisualSnapshot value;
    };
    std::vector<Record> records_;
    // Map::Get is an explicit carried publication. The tactical map_active
    // flag alone is not evidence of this pointer's existence.
    std::optional<bool> map_present_;
    std::uint8_t map_chapter_{};
    bool map_active_{};
};
}
