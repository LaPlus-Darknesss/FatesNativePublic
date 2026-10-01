#include "fates/map/native_height_population.hpp"
#include <algorithm>
#include <cmath>
namespace fates::map::native {
namespace {
float Add(float a,float b) noexcept {volatile float r=a+b;return r;}
float Sub(float a,float b) noexcept {volatile float r=a-b;return r;}
float Mul(float a,float b) noexcept {volatile float r=a*b;return r;}
bool Convertible(float v) noexcept {return std::isfinite(v)&&v>=-2147483648.0f&&v<2147483648.0f;}
bool Inside(HeightMapRange r,const HeightData& d) noexcept {return d.x>=r.min_x&&d.y>=r.min_y&&d.x<=r.max_x&&d.y<=r.max_y;}
void AddPoint(HeightMapRange& r,int x,int y) noexcept {
    r.min_x=std::min(r.min_x,x);r.min_y=std::min(r.min_y,y);r.max_x=std::max(r.max_x,x);r.max_y=std::max(r.max_y,y);
}
void AddBounds(std::array<float,6>& a,const std::array<float,6>& b) noexcept {
    // Preserve source operand selection on equality, including signed zero.
    for(unsigned i=0;i<3;++i){if(a[i]>b[i])a[i]=b[i];if(a[i+3]<=b[i+3])a[i+3]=b[i+3];}
}
}
FieldHeightSelection SelectFieldHeightSourceExact(const FieldHeightPart* part,int state,std::uint16_t flags) {
    if((flags&0x18u)!=8u)return {};
    if(!part||state<0||state>=3)return {HeightPopulationStatus::InvalidState};
    return {HeightPopulationStatus::Ok,part->lists[std::size_t(state)]};
}
void ClampHeightRangeExact(HeightMapRange& r) noexcept {
    r.min_x=std::max(r.min_x,0);r.min_y=std::max(r.min_y,0);r.max_x=std::min(r.max_x,31);r.max_y=std::min(r.max_y,31);
}
void AddHeightRangeExact(HeightMapRange& a,const HeightMapRange& b) noexcept {AddPoint(a,b.min_x,b.min_y);AddPoint(a,b.max_x,b.max_y);}
bool AddHeightBoundsRangeExact(HeightMapRange& range,const std::array<float,6>& b) noexcept {
    for(auto f:b)if(!std::isfinite(f))return false;
    std::array<int,4> q{};
    for(unsigned i=0;i<2;++i) {
        const unsigned axis=i*2;const auto center=Mul(Add(b[axis+3],b[axis]),0.5f);
        const auto low=Add(b[axis],25.0f),high=Sub(b[axis+3],25.0f);
        const auto a=Mul(center<low?center:low,0.02f),z=Mul(high<center?center:high,0.02f);
        if(!Convertible(a)||!Convertible(z))return false;q[i]=int(a);q[i+2]=int(z);
    }
    AddPoint(range,q[0],q[1]);AddPoint(range,q[2],q[3]);return true;
}
bool QueueHeightListRangeExact(HeightMapRange& pending,const FieldHeightList* list) noexcept {
    if(!list)return true;HeightMapRange r;if(!AddHeightBoundsRangeExact(r,list->bounds))return false;
    ClampHeightRangeExact(r);AddHeightRangeExact(pending,r);return true;
}
HeightPopulationStatus UpdateHeightPopulationExact(HeightMapGeometry& geometry,HeightMapRange requested,std::span<const WorldHeightContribution> objects) {
    // Preflight every potentially addressed record before clearing anything.
    for(const auto& object:objects) {
        if(!(object.flags&8u)||!object.world_list)continue;
        const auto& list=*object.world_list;HeightMapRange checked;
        if(!AddHeightBoundsRangeExact(checked,list.bounds))return HeightPopulationStatus::InvalidBounds;
        for(const auto& d:list.records)if(Inside(requested,d)&&(d.x<0||d.x>31||d.y<0||d.y>31||d.layer>1))return HeightPopulationStatus::InvalidRecord;
    }
    if(requested.min_x<=requested.max_x&&requested.min_y<=requested.max_y) {
        auto clear=requested;ClampHeightRangeExact(clear);
        for(int y=clear.min_y;y<=clear.max_y;++y)for(int x=clear.min_x;x<=clear.max_x;++x) {
            auto at=std::size_t((y*32+x)*2);ClearHeightDataExact(geometry.cells[at]);ClearHeightDataExact(geometry.cells[at+1]);
        }
    }
    for(const auto& object:objects) {
        if(!(object.flags&8u)||!object.world_list)continue;
        const auto& list=*object.world_list;
        for(const auto& d:list.records)if(Inside(requested,d))MergeWorldHeightRecordExact(geometry,d);
        AddBounds(geometry.bounds,list.bounds);AddHeightBoundsRangeExact(geometry.range,list.bounds);ClampHeightRangeExact(geometry.range);
    }
    return HeightPopulationStatus::Ok;
}
HeightPopulationStatus RebuildCurrentHeightPopulation(fates::runtime::native::NativeRuntime& r,std::span<const WorldHeightContribution> objects) {
    auto next=std::make_unique<HeightMapGeometry>();const auto result=UpdateHeightPopulationExact(*next,{0,0,31,31},objects);
    if(result!=HeightPopulationStatus::Ok)return result;
    return RestoreCurrentHeightGeometry(r,*next)==HeightStatus::Ok?HeightPopulationStatus::Ok:HeightPopulationStatus::InvalidBounds;
}
HeightPopulationStatus UpdateCurrentHeightPopulation(fates::runtime::native::NativeRuntime& r,HeightMapRange request,std::span<const WorldHeightContribution> objects) {
    const auto view=ReadCurrentHeightGeometry(r);
    if(view.status==HeightStatus::MissingGeometry)return HeightPopulationStatus::MissingGeometry;
    if(view.status!=HeightStatus::Ok)return HeightPopulationStatus::StaleGeometry;
    auto next=std::make_unique<HeightMapGeometry>(*view.geometry);const auto result=UpdateHeightPopulationExact(*next,request,objects);
    if(result!=HeightPopulationStatus::Ok)return result;
    return RestoreCurrentHeightGeometry(r,*next)==HeightStatus::Ok?HeightPopulationStatus::Ok:HeightPopulationStatus::InvalidBounds;
}
}
