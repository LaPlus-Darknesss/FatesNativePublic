#include "fates/map/native_field_geometry.hpp"
#include <type_traits>
namespace fates::map::native {
bool FieldGeometryInstance::QueueActiveRemoval(HeightMapRange& pending) const {
    return QueueHeightListRangeExact(pending,active_.height.list.get());
}
namespace {
template<class List,class Transform> FieldGeometryStatus Prepare(
    const std::shared_ptr<const List>& source,FieldGeometryBinding<List>& cache,
    FieldGeometryBinding<List>& active,Transform transform) {
    if(!source)return FieldGeometryStatus::Ok; // inactive source retains its cache
    List next;if(cache.list)next=*cache.list;else next.records.resize(source->records.size());
    const bool copied=[&]{if constexpr(std::is_same_v<List,FieldHeightList>)return CopyFieldHeightListExact(*source,next);else return CopyFieldPolygonListExact(*source,next);}();
    if(!copied)return FieldGeometryStatus::ShortSource;
    if(!transform(next))return FieldGeometryStatus::InvalidTransform;
    cache.list=std::make_shared<const List>(std::move(next));
    if(!cache.cache_identity)cache.cache_identity=std::make_shared<const FieldGeometryCacheIdentity>();
    active=cache;return FieldGeometryStatus::Ok;
}
}
FieldGeometryUpdate FieldGeometryInstance::Update(FieldGeometryRequest& input,HeightMapRange& pending) {
    if(!(input.flags&2u))return {};
    const auto fail=[](FieldGeometryStatus status){FieldGeometryUpdate r;r.status=status;return r;};
    FieldMatrix matrix;
    if(!MakeFieldPoseMatrixExact(input.pose,matrix))return fail(FieldGeometryStatus::InvalidTransform);
    if(part_&&input.level!=-1) {
        if(input.level<0||input.level>=4)return fail(FieldGeometryStatus::InvalidLevel);
        FieldMatrix level;if(!MakeFieldPoseMatrixExact(part_->level_poses[std::size_t(input.level)],level))return fail(FieldGeometryStatus::InvalidTransform);
        matrix=MultiplyFieldMatrixExact(matrix,level);
    }
    auto range=pending;if(!QueueHeightListRangeExact(range,active_.height.list.get()))return fail(FieldGeometryStatus::InvalidHeightBounds);
    auto caches=caches_;FieldGeometrySnapshot active;
    const bool enabled=(input.flags&0x18u)==8u;
    if(enabled) {
        if(!part_||input.state<0||input.state>=3)return fail(FieldGeometryStatus::InvalidState);
        const auto state=std::size_t(input.state);active.height.list=part_->lists[state];active.geometry.list=part_->geometry[state];active.collision.list=part_->collision[state];
        // Original predicate tests base components only, not the composed matrix.
        if(!IsIdentityFieldPoseExact(input.pose)) {
            auto status=Prepare(active.height.list,caches.height,active.height,[&](auto& list){return TransformFieldHeightListExact(list,matrix,list);});
            if(status!=FieldGeometryStatus::Ok)return fail(status);
            status=Prepare(active.geometry.list,caches.geometry,active.geometry,[&](auto& list){return TransformFieldPolygonListExact(list,matrix,list);});
            if(status!=FieldGeometryStatus::Ok)return fail(status);
            status=Prepare(active.collision.list,caches.collision,active.collision,[&](auto& list){return RoundFieldPolygonListExact(list,10,list)&&TransformFieldPolygonListExact(list,matrix,list);});
            if(status!=FieldGeometryStatus::Ok)return fail(status);
        }
        if(!QueueHeightListRangeExact(range,active.height.list.get()))return fail(FieldGeometryStatus::InvalidHeightBounds);
    }
    FieldGeometryUpdate result;result.changed=true;result.before=active_;result.after=active;
    result.events={FieldGeometryEvent::RemoveHeight,FieldGeometryEvent::RemoveGeometry,FieldGeometryEvent::RemoveCollision};
    if(enabled){result.events.push_back(FieldGeometryEvent::EntryHeight);result.events.push_back(FieldGeometryEvent::EntryGeometry);result.events.push_back(FieldGeometryEvent::EntryCollision);}
    result.events.push_back(FieldGeometryEvent::UpdateVisualTransform);
    active_=std::move(active);caches_=std::move(caches);pending=range;input.flags=std::uint16_t(input.flags&~2u);return result;
}
}
