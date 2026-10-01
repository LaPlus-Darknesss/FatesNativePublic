#pragma once
#include <cstdint>
#include <memory>
namespace fates::map::native {
struct HeightMapGeometry;
struct FieldWorldIdentity;
// Immutable, carried world-space height geometry. Field-object selection and
// transforms must be resolved by their owner before this state is published.
class NativeHeightState {
    friend struct HeightStateAccess;
    std::shared_ptr<const HeightMapGeometry> geometry_;
    bool map_active_{};
    std::uint8_t chapter_{};
    std::shared_ptr<const FieldWorldIdentity> field_owner_;
    std::uint64_t field_revision_{};
};
}
