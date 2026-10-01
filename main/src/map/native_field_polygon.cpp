#include "fates/map/native_field_transform.hpp"
#include <algorithm>
#include <bit>
#include <cmath>
#include <limits>
namespace fates::map::native {
namespace {
float Add(float a,float b) noexcept {volatile float r=a+b;return r;}
float Mul(float a,float b) noexcept {volatile float r=a*b;return r;}
float Div(float a,float b) noexcept {volatile float r=a/b;return r;}
bool Convertible(float v) noexcept {return std::isfinite(v)&&v>=-2147483648.0f&&v<2147483648.0f;}
bool Store(float v,std::int16_t& out) noexcept {
    if(!Convertible(v))return false;
    out=std::bit_cast<std::int16_t>(std::uint16_t(int(v)));return true;
}
std::array<float,6> EmptyBounds() noexcept {
    constexpr auto m=std::numeric_limits<float>::max();return {m,m,m,-m,-m,-m};
}
void AddPoint(std::array<float,6>& box,const std::array<std::int16_t,3>& point) noexcept {
    for(unsigned k=0;k<3;++k){const auto v=Mul(float(point[k]),0.0625f);if(v<box[k])box[k]=v;if(v>=box[k+3])box[k+3]=v;}
}
void AddBox(std::array<float,6>& box,const std::array<float,6>& other) noexcept {
    for(unsigned k=0;k<3;++k){if(other[k]<box[k])box[k]=other[k];if(other[k+3]>=box[k+3])box[k+3]=other[k+3];}
}
}
bool CopyFieldPolygonListExact(const FieldPolygonList& source,FieldPolygonList& out) {
    if(out.records.size()>source.records.size())return false;
    auto next=out;next.bounds=source.bounds;std::copy_n(source.records.begin(),next.records.size(),next.records.begin());out=std::move(next);return true;
}
bool TransformFieldPolygonListExact(const FieldPolygonList& source,const FieldMatrix& matrix,FieldPolygonList& out) {
    auto next=source;if(!TransformFieldBoundsExact(matrix,source.bounds,next.bounds))return false;
    for(auto& polygon:next.records)for(auto& vertex:polygon.vertices) {
        const auto p=TransformFieldPointExact(matrix,{Mul(float(vertex[0]),0.0625f),Mul(float(vertex[1]),0.0625f),Mul(float(vertex[2]),0.0625f)});
        if(!Store(Mul(p.x,16),vertex[0])||!Store(Mul(p.y,16),vertex[1])||!Store(Mul(p.z,16),vertex[2]))return false;
    }
    out=std::move(next);return true;
}
bool RoundFieldPolygonListExact(const FieldPolygonList& source,float step,FieldPolygonList& out) {
    if(!std::isfinite(step)||step==0)return false;
    const auto half=Mul(step,0.5f),inverse=Div(1,step);if(!std::isfinite(inverse))return false;
    auto next=source;next.bounds=EmptyBounds();
    for(auto& polygon:next.records)for(auto& vertex:polygon.vertices) {
        for(unsigned k=0;k<3;++k) {
            auto value=Mul(float(vertex[k]),0.0625f);
            if(k!=1){value=Mul(Add(value,half),inverse);if(!Convertible(value))return false;value=Mul(float(int(value)),step);}
            if(!Store(Mul(value,16),vertex[k]))return false;
        }
        // This belongs inside the vertex loop, as in 0x4EDD28..0x4EDDBC.
        auto bounds=EmptyBounds();for(const auto& p:polygon.vertices)AddPoint(bounds,p);AddBox(next.bounds,bounds);
    }
    out=std::move(next);return true;
}
}
