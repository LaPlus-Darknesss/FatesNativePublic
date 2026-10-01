#include "fates/map/native_height_geometry.hpp"
#include "fates/map/native_actor_position.hpp"
#include "fates/runtime/native_runtime.hpp"
#include <bit>
#include <cmath>
namespace fates::map::native {
namespace {
// ARM VFP operations round after each instruction, including the product of
// VMLA/VMLS. Volatile intermediates prevent contraction/reassociation here.
float Add(float a,float b) noexcept {volatile float r=a+b;return r;}
float Sub(float a,float b) noexcept {volatile float r=a-b;return r;}
float Mul(float a,float b) noexcept {volatile float r=a*b;return r;}
float Div(float a,float b) noexcept {volatile float r=a/b;return r;}
bool Finite(HeightVector q) noexcept {return std::isfinite(q.x)&&std::isfinite(q.y)&&std::isfinite(q.z);}
HeightVector Sub(HeightVector a,HeightVector b) noexcept {return {Sub(a.x,b.x),Sub(a.y,b.y),Sub(a.z,b.z)};}
HeightVector Cross(HeightVector a,HeightVector b) noexcept {return {Sub(Mul(a.y,b.z),Mul(a.z,b.y)),Sub(Mul(a.z,b.x),Mul(a.x,b.z)),Sub(Mul(a.x,b.y),Mul(a.y,b.x))};}
float Dot(HeightVector a,HeightVector b) noexcept {return Add(Add(Mul(a.x,b.x),Mul(a.y,b.y)),Mul(a.z,b.z));}
bool Truncatable(float f) noexcept {return std::isfinite(f)&&f>=-2147483648.0f&&f<2147483648.0f;}
std::int16_t LowShort(float f) noexcept {const auto bits=std::uint32_t(std::int32_t(f))&65535u;return std::int16_t(bits<32768?int(bits):int(bits)-65536);}
float Mean(const HeightData& d) noexcept {float v=Mul(float(d.heights[0]),0.0625f);for(unsigned i=1;i<4;++i)v=Add(v,Mul(float(d.heights[i]),0.0625f));return Mul(v,0.25f);}
bool Cell(int x,int y) noexcept {return x>=0&&x<32&&y>=0&&y<32;}
std::size_t Index(int x,int y,bool layer) noexcept {return std::size_t((y*32+x)*2+unsigned(layer));}
}
HeightMapGeometry::HeightMapGeometry() {
    for(int y=0;y<32;++y)for(int x=0;x<32;++x)for(unsigned layer=0;layer<2;++layer) {
        auto& d=cells[Index(x,y,layer!=0)];d.x=std::int8_t(x);d.y=std::int8_t(y);d.layer=std::uint8_t(layer);
    }
}
void ClearHeightDataExact(HeightData& d) noexcept {d.diagonal=2;d.heights.fill(0);}
void CommitHeightDataExact(HeightData& d,const HeightData& incoming) noexcept {
    if(d.diagonal==2||Mean(incoming)>Mean(d)){d.diagonal=incoming.diagonal;d.heights=incoming.heights;}
}
HeightVector HeightDataPositionExact(const HeightData& d,unsigned v) noexcept {
    return {Mul(Add(float(d.x),v>=2?1.0f:0.0f),50.0f),Mul(float(d.heights[v]),0.0625f),Mul(Add(float(d.y),(v&1)?1.0f:0.0f),50.0f)};
}
bool SetHeightPolygonPositionExact(HeightPolygon& p,unsigned v,HeightVector q) noexcept {
    if(v>=3)return false;const std::array<float,3> scaled{Mul(q.x,16.0f),Mul(q.y,16.0f),Mul(q.z,16.0f)};
    for(auto f:scaled)if(!Truncatable(f))return false;
    for(unsigned i=0;i<3;++i)p.vertices[v][i]=LowShort(scaled[i]);return true;
}
HeightVector HeightPolygonPositionExact(const HeightPolygon& p,unsigned v) noexcept {
    const auto& c=p.vertices[v];return {Mul(float(c[0]),0.0625f),Mul(float(c[1]),0.0625f),Mul(float(c[2]),0.0625f)};
}
bool SafeNormalizeHeightVectorExact(HeightVector* output,HeightVector v) noexcept {
    if(!output||(v.x==0.0f&&v.y==0.0f&&v.z==0.0f))return false;
    const auto square=Dot(v,v);
    if(std::bit_cast<std::uint32_t>(square)==0x3f800000u){*output=v;return true;}
    volatile float length=std::sqrt(square);const auto scale=Div(1.0f,length);
    *output={Mul(v.x,scale),Mul(v.y,scale),Mul(v.z,scale)};return true;
}
bool IntersectHeightSegmentExact(HeightVector a,HeightVector b,const std::array<HeightVector,3>& tri,HeightVector& out) noexcept {
    const auto ab=Sub(tri[1],tri[0]),ac=Sub(tri[2],tri[0]),qp=Sub(a,b),n=Cross(ab,ac);
    const auto d=Dot(qp,n);if(d<=0.0f)return false;
    const auto ap=Sub(a,tri[0]);const auto t=Dot(ap,n);if(t<0.0f||t>d)return false;
    const auto e=Cross(qp,ap);const auto v=Dot(ac,e);if(v<0.0f||v>d)return false;
    // Original final VNMLA adds negated accumulator to negated product.
    const auto w=Add(-Add(Mul(ab.x,e.x),Mul(ab.y,e.y)),-Mul(ab.z,e.z));
    if(w<0.0f||Add(v,w)>d)return false;
    const auto factor=-Mul(t,Div(1.0f,d));
    out={Add(a.x,Mul(qp.x,factor)),Add(a.y,Mul(qp.y,factor)),Add(a.z,Mul(qp.z,factor))};return true;
}
HeightVector HeightPolygonNormalExact(const HeightPolygon& p) noexcept {
    const auto v=HeightPolygonPositionExact(p,0);auto n=Cross(Sub(v,HeightPolygonPositionExact(p,1)),Sub(v,HeightPolygonPositionExact(p,2)));
    return SafeNormalizeHeightVectorExact(&n,n)?n:HeightVector{0.0f,1.0f,0.0f};
}
bool ValidHeightGeometry(const HeightMapGeometry& g) noexcept {
    const auto r=g.range;return r.min_x>r.max_x||r.min_y>r.max_y||(Cell(r.min_x,r.min_y)&&Cell(r.max_x,r.max_y));
}
void ClearHeightGeometryExact(HeightMapGeometry& g) noexcept {
    for(auto& d:g.cells)ClearHeightDataExact(d);ClearHeightDataExact(g.fallback);g.range={};
    const auto high=std::numeric_limits<float>::max();g.bounds={high,high,high,-high,-high,-high};
}
bool MergeWorldHeightRecordExact(HeightMapGeometry& g,const HeightData& d) noexcept {
    if(!Cell(d.x,d.y)||d.layer>1)return false;CommitHeightDataExact(g.cells[Index(d.x,d.y,d.layer!=0)],d);return true;
}
bool CommitUpperHeightExact(HeightMapGeometry& g,int x,int y,float offset) noexcept {
    if(!Cell(x,y))return false;auto next=g.cells[Index(x,y,true)];const auto& base=g.cells[Index(x,y,false)];
    for(unsigned i=0;i<4;++i){const auto v=Mul(Add(offset,Mul(float(base.heights[i]),0.0625f)),16.0f);if(!Truncatable(v))return false;next.heights[i]=LowShort(v);}
    g.cells[Index(x,y,true)]=next;return true;
}
HeightPolygonResult QueryHeightPolygonExact(const HeightMapGeometry& g,HeightVector q,bool layer) noexcept {
    if(!ValidHeightGeometry(g))return {HeightStatus::InvalidGeometry};if(!Finite(q))return {HeightStatus::InvalidValue};
    const auto xf=q.x<0.0f?Add(-1.0f,Mul(q.x,0.02f)):Mul(q.x,0.02f);
    const auto yf=q.z<0.0f?Add(-1.0f,Mul(q.z,0.02f)):Mul(q.z,0.02f);
    if(!Truncatable(xf)||!Truncatable(yf))return {HeightStatus::InvalidValue};
    const int x=int(xf),y=int(yf);const auto r=g.range;
    const auto& d=(x>=r.min_x&&y>=r.min_y&&x<=r.max_x&&y<=r.max_y)?g.cells[Index(x,y,layer)]:g.fallback;
    if(d.diagonal>1)return {HeightStatus::NoPolygon};
    const auto local_x=Sub(q.x,Mul(float(d.x),50.0f)),local_y=Sub(q.z,Mul(float(d.y),50.0f));
    const std::array<unsigned,3> vertices=d.diagonal==0?(local_y<=Sub(50.0f,local_x)?std::array<unsigned,3>{1,2,0}:std::array<unsigned,3>{1,3,2}):(local_y<=local_x?std::array<unsigned,3>{3,2,0}:std::array<unsigned,3>{1,3,0});
    HeightPolygonResult result{HeightStatus::Ok};for(unsigned i=0;i<3;++i)SetHeightPolygonPositionExact(result.polygon,i,HeightDataPositionExact(d,vertices[i]));return result;
}
HeightHitResult CalculateHeightHitExact(const HeightMapGeometry& g,HeightVector q,bool layer) noexcept {
    const auto poly=QueryHeightPolygonExact(g,q,layer);HeightHitResult result;
    if(poly.status==HeightStatus::NoPolygon)return result;
    if(poly.status!=HeightStatus::Ok)return {poly.status};
    std::array<HeightVector,3> vertices;for(unsigned i=0;i<3;++i)vertices[i]=HeightPolygonPositionExact(poly.polygon,i);
    HeightVector point;if(!IntersectHeightSegmentExact({q.x,1000.0f,q.z},{q.x,-1000.0f,q.z},vertices,point))return result;
    result.hit={2,0,point,HeightPolygonNormalExact(poly.polygon),vertices};return result;
}
MapHeightResult QueryMapHeightExact(const HeightMapGeometry& g,HeightVector q,bool layer) noexcept {
    const auto hit=CalculateHeightHitExact(g,{Mul(q.x,50.0f),Mul(q.y,50.0f),Mul(q.z,50.0f)},layer);
    return {hit.status,Mul(hit.hit.type?hit.hit.point.y:0.0f,0.02f)};
}
struct HeightStateAccess {
    using Runtime=fates::runtime::native::NativeRuntime;
    static CurrentHeightGeometryView Read(const Runtime& r) noexcept {
        const auto& s=r.game.scene_height;if(!s.geometry_)return {};
        if(s.map_active_!=r.game.map_active||s.chapter_!=r.game.campaign.current_chapter_index)return {HeightStatus::StaleGeometry};
        if(s.field_owner_!=r.game.field_scene.owner||s.field_revision_!=r.game.field_scene.terrain_revision)return {HeightStatus::StaleGeometry};
        return {HeightStatus::Ok,s.geometry_.get()};
    }
    static HeightStatus Restore(Runtime& r,const HeightMapGeometry& g) {
        if(!ValidHeightGeometry(g))return HeightStatus::InvalidGeometry;
        auto next=std::make_shared<const HeightMapGeometry>(g);auto& s=r.game.scene_height;
        s.geometry_=std::move(next);s.map_active_=r.game.map_active;s.chapter_=r.game.campaign.current_chapter_index;
        s.field_owner_=r.game.field_scene.owner;s.field_revision_=r.game.field_scene.terrain_revision;
        InvalidateCurrentActorPositions(r.game);return HeightStatus::Ok;
    }
    static void Invalidate(fates::runtime::native::NativeGameState& g) noexcept {g.scene_height.geometry_.reset();InvalidateCurrentActorPositions(g);}
};
HeightStatus RestoreCurrentHeightGeometry(fates::runtime::native::NativeRuntime& r,const HeightMapGeometry& g){return HeightStateAccess::Restore(r,g);}
void InvalidateCurrentHeightGeometry(fates::runtime::native::NativeGameState& g) noexcept {HeightStateAccess::Invalidate(g);}
CurrentHeightGeometryView ReadCurrentHeightGeometry(const fates::runtime::native::NativeRuntime& r) noexcept {return HeightStateAccess::Read(r);}
MapHeightResult QueryCurrentMapHeight(const fates::runtime::native::NativeRuntime& r,HeightVector q,bool layer) noexcept {
    const auto view=ReadCurrentHeightGeometry(r);return view.status==HeightStatus::Ok?QueryMapHeightExact(*view.geometry,q,layer):MapHeightResult{view.status};
}
}
