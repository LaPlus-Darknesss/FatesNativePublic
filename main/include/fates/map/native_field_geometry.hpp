#pragma once
#include "fates/map/native_field_transform.hpp"
#include "fates/map/native_height_population.hpp"
namespace fates::map::native {
// A cache's logical identity survives immutable data snapshots. Archive bindings
// instead use their shared source list as identity. These are not retail addresses.
struct FieldGeometryCacheIdentity {};
template<class List> struct FieldGeometryBinding {
    std::shared_ptr<const List> list;
    std::shared_ptr<const FieldGeometryCacheIdentity> cache_identity;
    bool SameIdentity(const FieldGeometryBinding& other) const noexcept {
        if(cache_identity||other.cache_identity)return cache_identity==other.cache_identity;
        return list==other.list;
    }
};
struct FieldGeometrySnapshot {
    FieldGeometryBinding<FieldHeightList> height;
    FieldGeometryBinding<FieldPolygonList> geometry,collision;
};
// Inputs carried by the FieldObject owner. Level is -1 or 0..3; state is 0..2
// when active. This API does not select LOD or imply successful visual loading.
struct FieldGeometryRequest {FieldPose pose;int state{};int level{-1};std::uint16_t flags{};};
enum class FieldGeometryStatus {Ok,InvalidState,InvalidLevel,InvalidTransform,ShortSource,InvalidHeightBounds};
enum class FieldGeometryEvent {RemoveHeight,RemoveGeometry,RemoveCollision,EntryHeight,EntryGeometry,EntryCollision,UpdateVisualTransform};
struct FieldGeometryUpdate {
    FieldGeometryStatus status{FieldGeometryStatus::Ok};
    bool changed{};
    FieldGeometrySnapshot before,after;
    // Ordered service boundary, including null registrations. Height events have
    // already updated the supplied range. ColsTree and visual services are external.
    std::vector<FieldGeometryEvent> events;
};
// Numeric UpdateDispos owner: immutable archive data, fixed-count private caches,
// active bindings and old/new height range queue. Not the whole FieldObject,
// resource loader, collision tree, field world, visual or actor lifetime.
class FieldGeometryInstance {
public:
    explicit FieldGeometryInstance(std::shared_ptr<const FieldHeightPart> part):part_(std::move(part)){}
    FieldGeometryInstance(const FieldGeometryInstance&)=delete;
    FieldGeometryInstance& operator=(const FieldGeometryInstance&)=delete;
    FieldGeometryInstance(FieldGeometryInstance&&) noexcept=default;
    FieldGeometryInstance& operator=(FieldGeometryInstance&&) noexcept=default;
    const FieldGeometrySnapshot& Active() const noexcept{return active_;}
    const FieldGeometrySnapshot& Caches() const noexcept{return caches_;}
    FieldGeometryUpdate Update(FieldGeometryRequest&,HeightMapRange& pending);
    // FieldObject::Free removes current bindings before freeing effects/resources,
    // then clears active pointers and finally its private caches. These separate
    // operations preserve that lifetime order; Free is not an inactive Update.
    bool QueueActiveRemoval(HeightMapRange& pending) const;
    void ClearActive() noexcept {active_={};}
    void ClearCaches() noexcept {caches_={};}
private:
    std::shared_ptr<const FieldHeightPart> part_;
    FieldGeometrySnapshot active_,caches_;
};
}
