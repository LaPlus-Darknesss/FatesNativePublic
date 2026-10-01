#include "fates/map/native_field_transform.hpp"
#include <bit>
#include <cmath>
#include <algorithm>
namespace fates::map::native {
namespace {
float Add(float a,float b) noexcept {volatile float r=a+b;return r;}
float Sub(float a,float b) noexcept {volatile float r=a-b;return r;}
float Mul(float a,float b) noexcept {volatile float r=a*b;return r;}
float F(std::uint32_t u) noexcept {return std::bit_cast<float>(u);}
bool Convertible(float v) noexcept {return std::isfinite(v)&&v>=-2147483648.0f&&v<2147483648.0f;}
template<std::size_t N> bool Finite(const std::array<float,N>& a) noexcept {return std::all_of(a.begin(),a.end(),[](float v){return std::isfinite(v);});}
#include "native_field_rotation_table.inc"
bool SinCos(float index,float& sine,float& cosine) noexcept {
    // The original repeatedly subtracts 65536 from the absolute index, then
    // uses the low eight integer bits and fractional remainder. Below 2^40
    // every such subtraction is exactly representable in binary32; fmod gives
    // the same remainder without a potentially enormous loop. At/above 2^40
    // the original subtraction can stop making progress, so refuse that domain.
    if(!std::isfinite(index)||std::fabs(index)>=0x1p40f)return false;
    const float absolute=std::fmod(std::fabs(index),65536.0f);
    const auto integer=static_cast<std::uint32_t>(absolute);
    const auto fraction=Sub(absolute,float(integer));const auto at=(integer&255u)*4u;
    sine=Add(F(RotationTable[at]),Mul(fraction,F(RotationTable[at+2])));
    cosine=Add(F(RotationTable[at+1]),Mul(fraction,F(RotationTable[at+3])));
    if(index<0)sine=-sine;return true;
}
float Acos(float x) noexcept {
    const auto u=std::bit_cast<std::uint32_t>(x),twice=u<<1;
    if(twice<0x66000000u)return F(0x3fc90fdb);
    float square,hi,lo;
    if(twice>0x7e000000u) {
        square=Mul(Sub(1.0f,std::fabs(x)),0.5f);
        float root=std::sqrt(square);hi=lo=0;
        if(u&0x80000000u){root=-root;hi=F(0x40490000);lo=F(0x3a7daa22);}
        x=Mul(root,-2.0f);
    } else {square=Mul(x,x);hi=F(0x3fc90000);lo=F(0x39fdaa22);}
    auto result=Sub(lo,x);x=Mul(x,square);
    auto p=Add(F(0x3cd88ac7),Mul(square,F(0x3d1cfe24)));
    p=Add(F(0x3d38671e),Mul(square,p));p=Add(F(0x3d99931b),Mul(square,p));p=Add(F(0x3e2aaaaf),Mul(square,p));
    return Add(Sub(result,Mul(x,p)),hi);
}
}
bool IsIdentityFieldPoseExact(const FieldPose& p) noexcept {
    return p.scale==std::array<float,3>{1,1,1}&&p.rotation_degrees==std::array<float,3>{0,0,0}&&p.position==std::array<float,3>{0,0,0};
}
bool RoundFieldPoseExact(FieldPose& p) noexcept {
    auto next=p;for(auto* group:{&next.scale,&next.rotation_degrees,&next.position})for(auto& v:*group){const auto scaled=Mul(v,8192.0f);if(!Convertible(scaled))return false;v=Mul(float(int(scaled)),0x1p-13f);}p=next;return true;
}
FieldMatrix MultiplyFieldMatrixExact(const FieldMatrix& left,const FieldMatrix& right) noexcept {
    FieldMatrix out;const auto& a=left.values;const auto& b=right.values;
    for(unsigned r=0;r<3;++r)for(unsigned c=0;c<4;++c){
        auto v=c==3?Add(a[r*4+3],Mul(b[c],a[r*4])):Mul(b[c],a[r*4]);
        v=Add(v,Mul(b[4+c],a[r*4+1]));out.values[r*4+c]=Add(v,Mul(b[8+c],a[r*4+2]));
    }return out;
}
HeightVector TransformFieldPointExact(const FieldMatrix& matrix,HeightVector p) noexcept {
    const auto& m=matrix.values;std::array<float,3> out{};
    for(unsigned r=0;r<3;++r){auto v=Add(m[r*4+3],Mul(m[r*4],p.x));v=Add(v,Mul(m[r*4+1],p.y));out[r]=Add(v,Mul(m[r*4+2],p.z));}return {out[0],out[1],out[2]};
}
bool MakeFieldPoseMatrixExact(const FieldPose& p,FieldMatrix& out) noexcept {
    if(!Finite(p.scale)||!Finite(p.rotation_degrees)||!Finite(p.position))return false;
    std::array<float,3> sine{},cosine{};
    for(unsigned i=0;i<3;++i)if(!SinCos(Mul(Mul(p.rotation_degrees[i],F(0x3c8efa35)),F(0x4222f983)),sine[i],cosine[i]))return false;
    const auto sx=sine[0],sy=sine[1],sz=sine[2],cx=cosine[0],cy=cosine[1],cz=cosine[2];
    const auto sxcz=Mul(sx,cz),cxcz=Mul(cx,cz),cxsz=Mul(cx,sz),sxsz=Mul(sx,sz);
    FieldMatrix rotation{{Mul(cz,cy),Add(-cxsz,Mul(sxcz,sy)),Add(sxsz,Mul(cxcz,sy)),0,
                          Mul(sz,cy),Add(cxcz,Mul(sxsz,sy)),Add(-sxcz,Mul(cxsz,sy)),0,
                          -sy,Mul(sx,cy),Mul(cx,cy),0}};
    const FieldMatrix scale{{p.scale[0],0,0,0,0,p.scale[1],0,0,0,0,p.scale[2],0}};
    const FieldMatrix translate{{1,0,0,p.position[0],0,1,0,p.position[1],0,0,1,p.position[2]}};
    const auto next=MultiplyFieldMatrixExact(translate,MultiplyFieldMatrixExact(rotation,scale));
    if(!Finite(next.values))return false;out=next;return true;
}
bool TransformFieldBoundsExact(const FieldMatrix& matrix,const std::array<float,6>& box,std::array<float,6>& out) noexcept {
    if(!Finite(matrix.values)||!Finite(box))return false;
    constexpr unsigned corners[8][3]={{0,4,2},{0,1,2},{3,1,2},{3,4,2},{0,4,5},{0,1,5},{3,1,5},{3,4,5}};
    std::array<float,6> next{};
    for(unsigned i=0;i<8;++i){const auto p=TransformFieldPointExact(matrix,{box[corners[i][0]],box[corners[i][1]],box[corners[i][2]]});const std::array<float,3> v{p.x,p.y,p.z};if(!Finite(v))return false;
        for(unsigned j=0;j<3;++j){if(i==0){next[j]=next[j+3]=v[j];continue;}if(v[j]<=next[j])next[j]=v[j];if(v[j]>next[j+3])next[j+3]=v[j];}
    }out=next;return true;
}
bool FieldVectorThetaExact(HeightVector a,HeightVector b,float& out) noexcept {
    if(!Finite(std::array<float,3>{a.x,a.y,a.z})||!Finite(std::array<float,3>{b.x,b.y,b.z}))return false;
    const auto dot=Add(Add(Mul(a.x,b.x),Mul(a.y,b.y)),Mul(a.z,b.z));if(!std::isfinite(dot))return false;
    const auto u=std::bit_cast<std::uint32_t>(dot);
    if(std::bit_cast<std::int32_t>(u)>=0x3f7ffffe)out=0;
    else if(u>=0xbf7ffffeu)out=F(0x40490fdb);
    else out=Acos(dot);return true;
}
bool TransformFieldHeightListExact(const FieldHeightList& source,const FieldMatrix& matrix,FieldHeightList& out) {
    if(!Finite(matrix.values))return false;
    auto direction_matrix=matrix;direction_matrix.values[3]=direction_matrix.values[7]=direction_matrix.values[11]=0;
    const auto direction=TransformFieldPointExact(direction_matrix,{1,0,0});float angle{};
    if(!FieldVectorThetaExact({1,0,0},direction,angle))return false;
    const auto q=Mul(Add(Mul(angle,F(0x42652ee0)),45.0f),F(0x3c360b61));if(!Convertible(q))return false;
    int turns=int(q);if(Sub(Mul(0,direction.x),Mul(1,direction.z))<0)turns=4-turns;
    auto next=source;if(!TransformFieldBoundsExact(matrix,source.bounds,next.bounds))return false;
    for(auto& d:next.records){const auto p=TransformFieldPointExact(matrix,{Mul(Add(float(d.x),0.5f),50),0,Mul(Add(float(d.y),0.5f),50)});
        const auto x=p.x<0?Add(-1.0f,Mul(p.x,0.02f)):Mul(p.x,0.02f),z=p.z<0?Add(-1.0f,Mul(p.z,0.02f)):Mul(p.z,0.02f);
        if(!Convertible(x)||!Convertible(z))return false;
        d.x=std::bit_cast<std::int8_t>(std::uint8_t(int(x)));d.y=std::bit_cast<std::int8_t>(std::uint8_t(int(z)));
        for(int i=0;i<turns;++i){const auto h=d.heights;d.heights={h[2],h[0],h[3],h[1]};d.diagonal=std::uint8_t(1u&~d.diagonal);}
        for(auto& h:d.heights){const auto v=Add(float(h),Mul(p.y,16));if(!Convertible(v))return false;h=std::bit_cast<std::int16_t>(std::uint16_t(int(v)));}
    }out=std::move(next);return true;
}
bool CopyFieldHeightListExact(const FieldHeightList& source,FieldHeightList& out) {
    if(out.records.size()>source.records.size())return false;
    // Copy metadata only after the bounds and all addressed source data exist.
    auto next=out;next.bounds=source.bounds;std::copy_n(source.records.begin(),next.records.size(),next.records.begin());out=std::move(next);return true;
}
}
