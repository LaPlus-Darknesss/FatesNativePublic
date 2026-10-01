#pragma once
#include "fates/map/native_unit_deployment.hpp"
#include "fates/map/native_danger_image_types.hpp"
#include <span>
namespace fates::map::native {
// Original trick bytes +08..+16, including signed geometry and unsigned ranges.
struct DangerCannonRecord {
    std::array<std::uint8_t,15> fields{};
    bool operator==(const DangerCannonRecord&) const=default;
};
struct DangerUnit {std::uint16_t slot{};std::uint8_t force{};std::uint32_t flags{};};
enum class DangerStatus : std::uint8_t {
    Ok,InvalidGeometry,MissingDeployment,MissingCannonUse,MissingMovementCell,
    MissingIntrinsicCategory,MissingIntrinsicItem,MissingIntrinsicRange,InvalidUnit,
    MissingPhase,MissingForceOrder,MissingScenario,StaleScenario,MissingConfiguration,
    MissingDefinition,MissingEnhance,MissingEligibility,MissingSkill,UnboundImage,StaleImage
};
struct DangerAddServices {
    virtual ~DangerAddServices()=default;
    virtual bool UnitMove(DeploymentFields&,int power,std::uint32_t flags,std::uint32_t extra)=0;
    virtual std::optional<bool> MountedCannonUsable(const DangerCannonRecord&)=0;
    // MoveImage::Get includes terrain flag8, not only the signed movement byte.
    virtual std::optional<int> MoveImageGet(const DeploymentFields&,int x,int y)=0;
    virtual std::optional<bool> IntrinsicCategory()=0;
    virtual std::optional<bool> IntrinsicItemAvailable()=0;
    virtual std::optional<int> IntrinsicInner()=0;
    virtual std::optional<int> IntrinsicOuter()=0;
    virtual std::optional<int> IntrinsicArea()=0;
};
struct DangerUpdateServices {
    virtual ~DangerUpdateServices()=default;
    virtual bool Allied(const DangerUnit&,std::uint8_t human_force);
    virtual DangerStatus Add(DangerImage&,const DangerUnit&)=0;
};
std::uint32_t DangerStaffFlagsExact(std::uint8_t selector) noexcept;
// Search2 with x=y=-1, type0, flags1, and a null Cannon receiver filter.
std::vector<DangerCannonRecord> EnumerateDangerCannonsExact(std::span<const DangerCannonRecord>);
DangerStatus AddDangerImageExact(DangerImage&,DeploymentFields&,TerrainImageGeometry,
    std::uint8_t configuration,std::uint32_t unit_flags,std::span<const DangerCannonRecord>,DangerAddServices&);
// Input order is Force0,1,2 list order. Each unit retains its actual force byte.
// Unknown flags are preserved. Both operations publish only on native success.
DangerStatus UpdateDangerImageExact(DangerImage&,std::uint8_t human_force,
    std::span<const DangerUnit>,DangerUpdateServices&);
struct CurrentDangerResult {
    DangerStatus status{DangerStatus::Ok};
    CurrentUnitDeploymentResult deployment{};
    std::uint16_t unit_slot{0xffff};
    std::uint32_t added_units{},mounted_cannons{};
};
CurrentDangerResult BuildCurrentDangerImage(DangerImage&,const fates::runtime::native::NativeRuntime&,
    std::optional<std::uint8_t> configuration);
struct CurrentDangerView {DangerStatus status{DangerStatus::UnboundImage};const DangerImage* image{};};
// Initialization owns the original four zero planes and zero flag word. Refresh
// is explicit. Read checks current semantic dependencies and never regenerates
// movement or threat planes. Slot generations prevent same-Person slot reuse.
void InitializeCurrentDangerImage(fates::runtime::native::NativeGameState&);
void InvalidateCurrentDangerImage(fates::runtime::native::NativeGameState&) noexcept;
DangerStatus SetCurrentWholeDanger(fates::runtime::native::NativeGameState&,bool) noexcept;
CurrentDangerResult RefreshCurrentDangerImage(fates::runtime::native::NativeRuntime&);
CurrentDangerView ReadCurrentDangerImage(const fates::runtime::native::NativeRuntime&);
}
