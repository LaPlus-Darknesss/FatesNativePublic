#include "fates/map/native_field_configuration.hpp"
#include "fates/runtime/native_archive.hpp"
#include <algorithm>
#include <bit>

namespace fates::map::native {
namespace {
std::uint32_t Word(std::span<const std::uint8_t> b,std::size_t at) noexcept {
    return std::uint32_t(b[at])|(std::uint32_t(b[at+1])<<8)|(std::uint32_t(b[at+2])<<16)|(std::uint32_t(b[at+3])<<24);
}
}
float FieldConfiguration::TimeZone() const noexcept {return std::bit_cast<float>(Word(numeric,0x14));}
bool DecodeFieldConfiguration(std::span<const std::uint8_t> b,std::shared_ptr<const FieldConfiguration>& output) {
    const auto header=fates::runtime::native::FindArchiveLabel(b,"\x83\x74\x83\x42\x81\x5b\x83\x8b\x83\x68\x90\xdd\x92\xe8");
    if(header.status!=fates::runtime::native::ArchiveLabelStatus::Found)return false;
    const auto data_end=std::size_t(Word(b,4)&~3u)+0x20;
    const auto fits=[&](std::size_t at,std::size_t count){return at>=0x20&&at<=data_end&&count<=data_end-at;};
    if(!fits(header.offset,12))return false;
    const auto pointer=Word(b,header.offset+8);
    if(!pointer){output.reset();return true;}
    const auto record=std::size_t(pointer)+0x20;
    if(pointer%4||!fits(record,196))return false;
    auto next=std::make_shared<FieldConfiguration>();std::size_t at=record;
    for(auto* value:{&next->field_name,&next->environment_name,&next->sky_name,&next->switch_field,&next->block_object,&next->environment_sound}) {
        const auto p=Word(b,at);at+=4;if(!p)continue;
        const auto start=std::size_t(p)+0x20;if(start>=b.size())return false;
        auto end=start;while(end<b.size()&&b[end])++end;if(end==b.size())return false;
        *value=std::string(reinterpret_cast<const char*>(b.data()+start),end-start);
    }
    std::copy_n(b.begin()+record+0x18,172,next->numeric.begin());output=std::move(next);return true;
}
std::string FieldConfigurationPath(std::string_view name) {
    std::string result="GameData/Field/"+std::string(name.substr(0,name.find('\0')))+".bin";
    result.resize(std::min<std::size_t>(result.size(),63));return result;
}
bool FieldConfigurationOwner::Load(std::string_view name) {
    const auto requested=FieldConfigurationPath(name);const bool exists=backend_.ConfigurationExists(requested);
    auto read=backend_.ReadConfiguration(exists?requested:"GameData/Field/Dummy.bin",resource_);
    resource_=std::move(read.resource);if(read.ready)configuration_=std::move(read.configuration);return exists;
}
void FieldConfigurationOwner::Free() {backend_.ReleaseConfiguration(resource_);configuration_.reset();}
}
