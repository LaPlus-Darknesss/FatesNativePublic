#pragma once
#include <cstdint>
#include <optional>
#include <vector>
namespace fates::map::native {
struct ActorPositionVector {float x{},y{},z{};bool operator==(const ActorPositionVector&) const=default;};
struct ActorPairOffset {float x{},z{};bool operator==(const ActorPairOffset&) const=default;};
// Current scene globals are carried state. BSS zeroes are not their initialized
// values. Only reached public flag2/4 lanes require an offset.
struct ActorPositionOffsets {
    std::optional<ActorPairOffset> lead,partner;
    bool operator==(const ActorPositionOffsets&) const=default;
};
class NativeActorPositionState {
    friend struct ActorPositionAccess;
    struct Record {
        std::uint16_t slot{},person{};
        std::uint64_t generation{};
        std::int16_t x{},y{};
        std::uint32_t pair_flags{};
        bool value_bound{};
        bool map_active{};
        std::uint8_t chapter{};
        ActorPositionOffsets offsets;
        ActorPositionVector position;
    };
    std::vector<Record> records_;
};
}
