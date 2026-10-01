#include "fates/cmvm/native_script_functions.hpp"

namespace fates::cmvm::native {
namespace {
std::uint32_t U32(std::span<const std::uint8_t> b,std::size_t at) {
    return std::uint32_t(b[at]) | (std::uint32_t(b[at+1])<<8u) |
        (std::uint32_t(b[at+2])<<16u) | (std::uint32_t(b[at+3])<<24u);
}
std::uint16_t U16(std::span<const std::uint8_t> b,std::size_t at) {
    return static_cast<std::uint16_t>(unsigned(b[at]) | (unsigned(b[at+1])<<8u));
}
bool ValidIdentifier(std::string_view name) {
    return name.size()<=1024 && name.find('\0')==std::string_view::npos;
}
}
const ScriptFunctionInfo* ScriptFunctionRef::info() const noexcept {
    return table_ ? &table_->functions()[index_] : nullptr;
}
ScriptFunctionStatus ScriptFunctionTable::Read(std::shared_ptr<const event::native::PhaseEventArchive> archive,
    std::shared_ptr<const ScriptFunctionTable>& out) {
    if(!archive)return ScriptFunctionStatus::NullArchive;
    auto next=std::shared_ptr<ScriptFunctionTable>(new ScriptFunctionTable);
    const auto b=archive->bytes();
    const auto table=std::size_t(U32(b,0x1c));
    next->functions_.reserve(archive->function_count());
    for(std::uint32_t index=0;index<archive->function_count();++index) {
        // PhaseEventArchive already admitted the terminated table and every24-byte record.
        const auto record=U32(b,table+index*4u);
        // CmGetNextFunctionTyped resumes from this signed16-bit index. Do not
        // silently use table position when a malformed record disagrees.
        if(U16(b,record+12)!=index)return ScriptFunctionStatus::InvalidIndex;
        const auto code=U32(b,record+4);
        if(code<0x28 || code>=b.size())return ScriptFunctionStatus::InvalidCode;
        ScriptFunctionInfo info{static_cast<std::uint16_t>(index),U16(b,record+10),
            b[record+8],b[record+9],record,code,{}};
        const auto name=std::size_t(U32(b,record+0x10));
        if(name) {
            if(name<0x28 || name>=b.size())return ScriptFunctionStatus::InvalidName;
            auto end=name;
            while(end<b.size() && b[end]!=0 && end-name<=1024)++end;
            if(end-name>1024)return ScriptFunctionStatus::LimitExceeded;
            if(end==b.size())return ScriptFunctionStatus::InvalidName;
            info.name=std::string(reinterpret_cast<const char*>(b.data()+name),end-name);
        }
        next->functions_.push_back(std::move(info));
    }
    next->archive_=std::move(archive);out=std::move(next);
    return ScriptFunctionStatus::Ok;
}
ScriptFunctionStatus ScriptFunctionTable::Get(std::size_t index,ScriptFunctionRef& out) const {
    if(index>=functions_.size())return ScriptFunctionStatus::InvalidIndex;
    ScriptFunctionRef next;next.table_=shared_from_this();next.index_=static_cast<std::uint16_t>(index);
    out=std::move(next);return ScriptFunctionStatus::Ok;
}
ScriptLookupStatus ScriptFunctionRegistry::ReadSnapshot(std::uint32_t count,
    std::span<const ScriptSymbolBinding> bindings,std::shared_ptr<const ScriptFunctionRegistry>& out) {
    if(count==0 || count>65536)return ScriptLookupStatus::InvalidBucketCount;
    if(bindings.size()>65536)return ScriptLookupStatus::LimitExceeded;
    auto next=std::shared_ptr<ScriptFunctionRegistry>(new ScriptFunctionRegistry(count));
    for(const auto& binding:bindings) {
        if(!ValidIdentifier(binding.identifier))return ScriptLookupStatus::InvalidBinding;
        if(binding.kind==ScriptSymbolKind::Script) {
            const auto* function=binding.function.info();
            if(!function || !function->name || *function->name!=binding.identifier)
                return ScriptLookupStatus::InvalidBinding;
        } else if(binding.kind!=ScriptSymbolKind::UnimplementedNative || binding.function)
            return ScriptLookupStatus::InvalidBinding;
        if(!next->registry_.Append(binding.identifier.c_str(),binding))return ScriptLookupStatus::InvalidBinding;
    }
    out=std::move(next);return ScriptLookupStatus::Ok;
}
ScriptLookupResult ScriptFunctionRegistry::Find(std::string_view identifier) const {
    if(!ValidIdentifier(identifier))return {ScriptLookupStatus::InvalidIdentifier,{},{}};
    const std::string key(identifier);
    const auto* entry=registry_.Find(key.c_str());
    if(!entry)return {};
    return {entry->value.kind==ScriptSymbolKind::Script ? ScriptLookupStatus::FoundScript : ScriptLookupStatus::NativeOwnerRequired,
        entry->identifier,entry->value.function};
}
}
