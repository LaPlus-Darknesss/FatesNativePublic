#pragma once
#include "fates/map/native_actor_position.hpp"
#include "fates/map/native_danger_image.hpp"
#include "fates/map/native_terrain_image.hpp"
namespace fates::map::native {
struct PositionPublicationUnit {std::uint16_t slot{};std::uint8_t force{};std::uint32_t flags{};};
enum class PositionPublicationStatus : std::uint8_t {
    Ok,Ignored,InvalidCoordinates,MissingUnit,MissingImage,MissingOccupant,
    MissingActorPresence,MissingActorPosition,MissingDanger,MissingRescue
};
struct PositionPublicationServices {
    virtual ~PositionPublicationServices()=default;
    virtual std::optional<PositionPublicationUnit> UnitForKey(int key)=0;
    virtual std::optional<std::uint8_t> OccupantKey(int x,int y)=0;
    virtual std::optional<std::uint16_t> OccupantUnit(int x,int y)=0;
    virtual bool MapPresent()=0;
    virtual bool DeleteImage(std::uint16_t)=0;
    virtual bool WriteCoordinates(std::uint16_t,std::uint8_t,std::uint8_t)=0;
    virtual std::optional<bool> HasActor(std::uint16_t)=0;
    virtual bool UpdateActor(std::uint16_t,bool update_partner)=0;
    virtual bool AddImage(std::uint16_t,bool selected)=0;
    virtual bool UpdateDanger()=0;
    virtual std::optional<std::pair<int,int>> RescuePosition(std::uint16_t,int,int,bool allow_origin,bool prefer_unit_origin)=0;
};
// Exact event ordering in a native valid 32x32 backing domain. Ignored admission
// is distinct from missing native services. The caller stages mutable services.
PositionPublicationStatus PublishUnitPositionExact(int pool_key,int x,int y,PositionPublicationServices&);
PositionPublicationStatus PublishUnitPositionFreeSpaceExact(int pool_key,int x,int y,PositionPublicationServices&);
struct CurrentPositionPublicationResult {
    PositionPublicationStatus status{PositionPublicationStatus::Ok};
    UnitImageStatus image_status{UnitImageStatus::Ok};
    ActorPositionStatus actor_status{ActorPositionStatus::Ok};
    CurrentDangerResult danger{};
    CurrentTacticalRescueResult rescue{};
};
// Pool keys are original one-based IDs. Complete game-owned state publishes only
// after every reached actor/image/Danger dependency succeeds. Queries are read-only.
CurrentPositionPublicationResult PublishCurrentUnitPosition(fates::runtime::native::NativeRuntime&,
    int pool_key,int x,int y,const CurrentActorHeightQuery* scene=nullptr);
CurrentPositionPublicationResult PublishCurrentUnitPositionFreeSpace(fates::runtime::native::NativeRuntime&,
    int pool_key,int x,int y,const CurrentActorHeightQuery* scene=nullptr);
}
