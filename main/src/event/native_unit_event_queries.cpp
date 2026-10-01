#include "fates/event/native_unit_event_queries.hpp"
#include "fates/runtime/native_runtime.hpp"

namespace fates::event::native {
using S=UnitEventQueryStatus;
S NativeUnitEventQueries::Bind(std::shared_ptr<runtime::native::NativeRuntime> runtime,
    std::shared_ptr<NativeUnitEventQueries>& out) {
    if(!runtime)return S::NullRuntime;
    auto next=std::shared_ptr<NativeUnitEventQueries>(new NativeUnitEventQueries);
    next->runtime_=std::move(runtime);out=std::move(next);return S::Ok;
}
S NativeUnitEventQueries::Validate() const noexcept {return retired_?S::Retired:S::Ok;}
UnitEventQueryResult NativeUnitEventQueries::GetByPid(std::string_view pid) const {
    UnitEventQueryResult result;result.status=Validate();if(result.status!=S::Ok)return result;
    if(pid.find('\0')!=std::string_view::npos){result.status=S::InvalidName;return result;}
    const auto& definitions=runtime_->definitions;
    if(!definitions.person_archives_known()) {
        result.status=S::StateUnavailable;result.lookup.status=runtime::native::UnitPersonLookupStatus::UnknownPersonArchives;
        return result;
    }
    // Name resolution remains a bounded native Person projection. Ambiguous
    // original hash/lifetime cases need the shared archive registry owner.
    const auto person=definitions.ResolveUnambiguousPersonName(pid);
    if(!person){result.status=S::StateUnavailable;return result;}
    // PROVEN 003A8390: a known absent name returns zero without touching the pool.
    if(!*person)return result;
    result.lookup=runtime::native::FindUnitFromPerson(definitions,runtime_->game,*person);
    if(result.lookup.status!=runtime::native::UnitPersonLookupStatus::Ok)result.status=S::StateUnavailable;
    else if(result.lookup.slot)result.value=static_cast<std::int32_t>(*result.lookup.slot)+1;
    return result;
}
}
