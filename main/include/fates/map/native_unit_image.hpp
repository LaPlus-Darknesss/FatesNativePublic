#pragma once
#include "fates/map/native_unit_image_state.hpp"
#include "fates/map/native_rescue_position.hpp"
#include <span>
namespace fates::runtime::native {struct NativeGameState;struct NativeRuntime;}
namespace fates::map::native {
struct UnitImageUnit {
    std::uint8_t key{};
    std::int8_t x{},y{};
    std::uint32_t flags{};
};
enum class UnitImageStatus : std::uint8_t {
    Ok,InvalidDimensions,InvalidUnit,InvalidCoordinates,MissingForceOrder,
    UnboundImage,StaleImage,InvalidOccupant,InvalidTerrainBounds
};
// PROVEN: Add overwrites, Delete clears only the matching key. Each -1 coordinate
// independently selects the stored signed byte. Full bounds, not active bounds,
// apply. Add's selected=true bypasses the public 0x24004 rejection mask.
UnitImageStatus AddUnitImageExact(UnitImageGrid&,UnitImageUnit,bool selected,int x=-1,int y=-1) noexcept;
UnitImageStatus DeleteUnitImageExact(UnitImageGrid&,UnitImageUnit,int x=-1,int y=-1) noexcept;
// Ordered input must concatenate Force0,1,2 in original head-to-tail order.
// Last eligible writer wins overlaps. Clears all 1024 cells, including padding.
UnitImageStatus RebuildUnitImageExact(UnitImageGrid&,std::span<const UnitImageUnit>) noexcept;
// GetUnit reads the backing plane without consulting full or active bounds.
// This native API rejects coordinates outside the 32x32 valid backing domain.
std::optional<std::uint8_t> UnitImageKeyAt(const UnitImageGrid&,int x,int y) noexcept;

// Native game-state ownership. Rebuild requires verified order for nonempty
// tactical forces. Restore accepts an explicitly supplied image, not a save or
// an inferred reconstruction. Neither changes Unit fields, RNG or force lists.
UnitImageStatus RebuildCurrentUnitImage(fates::runtime::native::NativeGameState&,int width,int height);
UnitImageStatus RestoreCurrentUnitImage(fates::runtime::native::NativeGameState&,const UnitImageGrid&);
void InvalidateCurrentUnitImage(fates::runtime::native::NativeGameState&) noexcept;
struct UnitImageView {
    UnitImageStatus status{UnitImageStatus::UnboundImage};
    const UnitImageGrid* image{};
};
// Valid for the current call only. Relevant unit fields, generation/identity and
// force order and map/chapter context must still match. HP, defeated and
// has_position are not image gates. Owners must explicitly invalidate at an
// unobserved map teardown/recreation; these bindings do not infer that lifetime.
UnitImageView ReadCurrentUnitImage(const fates::runtime::native::NativeGameState&) noexcept;
UnitImageStatus AddCurrentUnitToImage(fates::runtime::native::NativeGameState&,
    std::uint16_t slot,bool selected,int x=-1,int y=-1) noexcept;
UnitImageStatus DeleteCurrentUnitFromImage(fates::runtime::native::NativeGameState&,
    std::uint16_t slot,int x=-1,int y=-1) noexcept;
struct CurrentImageRescueResult {
    UnitImageStatus image_status{UnitImageStatus::UnboundImage};
    RescuePositionResult position{};
};
// Compose owned current occupancy with explicitly resolved current terrain.
// Does not claim terrain lifetime, actor/Danger effects or position publication.
CurrentImageRescueResult QueryRescuePositionWithCurrentImage(
    const fates::runtime::native::NativeRuntime&,std::uint16_t slot,
    const RescueTerrainImage&,int origin_x,int origin_y,bool allow_origin,bool prefer_unit_origin);
}
