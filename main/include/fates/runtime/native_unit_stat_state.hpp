#pragma once
#include "fates/support/native_family_support.hpp"
#include <array>
#include <cstdint>
#include <optional>
namespace fates::runtime::native {
// Shared carried Family/Edit facts. Support and capability queries consume this
// same owner; null payloads are known absence, distinct from an unbound state.
struct UnitEditFaceState {
    std::uint8_t gender{},body_type{},face_index{}; // original Edit20/21/22
    bool operator==(const UnitEditFaceState&) const=default;
};
struct UnitLineageSnapshot {
    std::optional<fates::support::native::SupportFamilyState> family;
    std::optional<fates::support::native::SupportEditState> edit;
    // The same current Edit whose boon/bane facts are above. Unknown name is
    // distinct from known empty. Original SetName owns13 raw UTF16 words;
    // no host wchar_t, localization, or serialized save layout enters here.
    std::optional<std::array<char16_t,13>> edit_name;
    std::optional<UnitEditFaceState> edit_face;
    bool operator==(const UnitLineageSnapshot&) const=default;
};
struct NativeUnitLineageState {
    bool bound{};
    std::uint16_t person_id{};
    UnitLineageSnapshot value{};
    std::uint64_t revision{};
};
struct UnitCapabilitySnapshot {
    std::array<std::int8_t,8> stored{};        // original current Unit+C8 lanes
    std::array<std::int8_t,8> limit_changes{}; // Unit+D0
    std::array<std::int8_t,8> penalties{};     // Unit+E0, gated by public40000000
    bool operator==(const UnitCapabilitySnapshot&) const=default;
};
struct NativeUnitCapabilityState {
    bool bound{};
    std::uint16_t person_id{},job_id{};
    UnitCapabilitySnapshot value{};
    std::uint64_t revision{};
};
// Current Unit+12C is independent of the existing map-end-owned12D penalty.
// Unknown carried adjustment is not zero; pool clear resets this binding.
struct NativeUnitMovementState {
    bool bound{};
    std::uint16_t person_id{};
    std::int8_t adjustment{};
    std::uint64_t revision{};
};
}
