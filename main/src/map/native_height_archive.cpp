#include "fates/map/native_height_archive.hpp"
#include "fates/runtime/native_archive.hpp"
#include <bit>
#include <cmath>
#include <unordered_map>
namespace fates::map::native {
bool DecodeFieldHeightArchive(std::span<const std::uint8_t> b,FieldHeightArchive& output) {
    // MapDataFile::TryRead compares MapHeader against the literal at 0x19ED9C.
    // Paragon's 248-byte FieldPart schema matches that admitted format.
    const auto version=fates::runtime::native::ReadArchiveLabeledWord(b,"MapHeader");
    if(!version||*version!=0x20140623u)return false;
    const auto label=fates::runtime::native::ReadArchiveLabeledOffset(b,"PartsList");if(!label)return false;
    const auto word=[&](std::size_t at){return std::uint32_t(b[at])|(std::uint32_t(b[at+1])<<8)|(std::uint32_t(b[at+2])<<16)|(std::uint32_t(b[at+3])<<24);};
    const auto data_end=std::size_t(word(4)&~3u)+0x20;
    const auto fits=[&](std::size_t at,std::size_t count){return at>=0x20&&at<=data_end&&count<=data_end-at;};
    const auto pointer=[&](std::uint32_t p){return std::size_t(p)+0x20;};
    const auto s8=[](unsigned v){return std::int8_t(v<128?int(v):int(v)-256);};
    const auto s16=[&](std::size_t at){const unsigned v=unsigned(b[at])|(unsigned(b[at+1])<<8);return std::int16_t(v<32768?int(v):int(v)-65536);};
    if(!fits(*label,8))return false;
    const auto count=std::size_t(word(*label));const auto parts_pointer=word(*label+4);const auto parts=pointer(parts_pointer);
    if(count&&(!parts_pointer||parts_pointer%4||!fits(parts,count*248)))return false;
    FieldHeightArchive next;std::unordered_map<std::uint32_t,std::shared_ptr<const FieldHeightList>> lists;
    std::unordered_map<std::uint32_t,std::shared_ptr<const FieldPolygonList>> polygons;
    std::unordered_map<std::uint32_t,std::shared_ptr<const FieldEffectPlacementList>> effects;
    const auto nullable_string=[&](std::uint32_t p,std::optional<std::string>& result){
        if(!p){result.reset();return true;}
        const auto start=pointer(p);if(start>=b.size())return false;
        auto end=start;while(end<b.size()&&b[end])++end;if(end==b.size())return false;
        result=std::string(reinterpret_cast<const char*>(b.data()+start),end-start);return true;
    };
    for(std::size_t i=0;i<count;++i) {
        const auto at=parts+i*248;FieldHeightPart part;const auto name_pointer=word(at);const auto name=pointer(name_pointer);
        if(!name_pointer||name>=b.size())return false;auto end=name;while(end<b.size()&&b[end])++end;if(end==b.size())return false;
        part.name.assign(reinterpret_cast<const char*>(b.data()+name),end-name);
        const auto bch_pointer=word(at+4);part.type=b[at+8];
        for(unsigned j=0;j<6;++j)part.bounds[j]=std::bit_cast<float>(word(at+12+j*4));
        for(unsigned j=0;j<4;++j)part.model_indices[j]=s8(b[at+36+j]);
        if(bch_pointer){
            const auto bch=pointer(bch_pointer);if(bch>=b.size())return false;
            auto bch_end=bch;while(bch_end<b.size()&&b[bch_end])++bch_end;if(bch_end==b.size())return false;
            part.bch_container=std::string(reinterpret_cast<const char*>(b.data()+bch),bch_end-bch);
        }
        for(unsigned level=0;level<4;++level) {
            auto offset=at+40+36*level;
            for(auto* group:{&part.level_poses[level].scale,&part.level_poses[level].rotation_degrees,&part.level_poses[level].position})
                for(auto& value:*group){value=std::bit_cast<float>(word(offset));offset+=4;}
        }
        for(unsigned lane=0;lane<3;++lane) {
            const auto p=word(at+184+lane*4);if(!p)continue;
            if(const auto found=lists.find(p);found!=lists.end()){part.lists[lane]=found->second;continue;}
            const auto h=pointer(p);if(p%4||!fits(h,32))return false;
            auto list=std::make_shared<FieldHeightList>();list->archive_offset=p;
            for(unsigned j=0;j<6;++j){list->bounds[j]=std::bit_cast<float>(word(h+8+j*4));if(!std::isfinite(list->bounds[j]))return false;}
            const auto n=std::size_t(word(h));const auto records_pointer=word(h+4);const auto records=pointer(records_pointer);
            if(n&&(!records_pointer||records_pointer%4||!fits(records,n*12)))return false;
            list->records.reserve(n);
            for(std::size_t j=0;j<n;++j) {
                const auto d=records+12*j;HeightData value{s8(b[d]),s8(b[d+1]),b[d+2],b[d+3]};
                for(unsigned k=0;k<4;++k)value.heights[k]=s16(d+4+k*2);
                list->records.push_back(value);
            }
            part.lists[lane]=list;lists.emplace(p,std::move(list));
        }
        for(unsigned lane=0;lane<6;++lane) {
            const auto p=word(at+196+lane*4);if(!p)continue;
            auto& destination=lane<3?part.geometry[lane]:part.collision[lane-3];
            if(const auto found=polygons.find(p);found!=polygons.end()){destination=found->second;continue;}
            const auto h=pointer(p);if(p%4||!fits(h,32))return false;
            auto list=std::make_shared<FieldPolygonList>();list->archive_offset=p;
            for(unsigned j=0;j<6;++j){list->bounds[j]=std::bit_cast<float>(word(h+8+j*4));if(!std::isfinite(list->bounds[j]))return false;}
            const auto n=std::size_t(word(h));const auto records_pointer=word(h+4);const auto records=pointer(records_pointer);
            if(n&&(!records_pointer||records_pointer%4||!fits(records,n*20)))return false;
            list->records.reserve(n);
            for(std::size_t j=0;j<n;++j) {
                const auto d=records+20*j;HeightPolygon value;
                for(unsigned v=0;v<3;++v)for(unsigned k=0;k<3;++k)value.vertices[v][k]=s16(d+v*6+k*2);
                value.attribute=std::uint16_t(unsigned(b[d+18])|(unsigned(b[d+19])<<8));
                list->records.push_back(value);
            }
            destination=list;polygons.emplace(p,std::move(list));
        }
        for(unsigned lane=0;lane<3;++lane) {
            const auto p=word(at+220+lane*4);if(!p)continue;
            if(const auto found=effects.find(p);found!=effects.end()){part.effects[lane]=found->second;continue;}
            const auto h=pointer(p);if(p%4||!fits(h,8))return false;
            const auto n=std::size_t(word(h));const auto records_pointer=word(h+4);const auto records=pointer(records_pointer);
            if(n&&(!records_pointer||records_pointer%4||!fits(records,n*44)))return false;
            auto list=std::make_shared<FieldEffectPlacementList>();list->archive_offset=p;list->records.reserve(n);
            for(std::size_t j=0;j<n;++j) {
                const auto d=records+j*44;FieldEffectPlacement value;
                if(!nullable_string(word(d),value.name)||!nullable_string(word(d+4),value.label))return false;
                auto offset=d+8;
                for(auto* group:{&value.pose.scale,&value.pose.rotation_degrees,&value.pose.position})
                    for(auto& component:*group){component=std::bit_cast<float>(word(offset));offset+=4;}
                list->records.push_back(std::move(value));
            }
            part.effects[lane]=list;effects.emplace(p,std::move(list));
        }
        next.parts.push_back(std::move(part));
    }
    output=std::move(next);return true;
}
}
