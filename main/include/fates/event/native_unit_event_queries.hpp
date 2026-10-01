#pragma once
#include "fates/runtime/native_unit_pool.hpp"
#include <memory>
#include <string_view>

namespace fates::runtime::native {struct NativeRuntime;}
namespace fates::event::native {
enum class UnitEventQueryStatus : std::uint8_t {Ok,NullRuntime,Retired,InvalidName,StateUnavailable};
struct UnitEventQueryResult {
    UnitEventQueryStatus status{UnitEventQueryStatus::Ok};
    std::int32_t value{};
    runtime::native::UnitPersonLookupResult lookup;
};
// Concrete read-only Unit services. They share the live runtime and do not bind
// a tactical phase or PlayerStateProvider flag bank merely to read Unit records.
class NativeUnitEventQueries final {
public:
    static UnitEventQueryStatus Bind(std::shared_ptr<runtime::native::NativeRuntime>,
        std::shared_ptr<NativeUnitEventQueries>&);
    UnitEventQueryStatus Validate() const noexcept;
    UnitEventQueryResult GetByPid(std::string_view) const;
    void Retire() noexcept {retired_=true;}
    const std::shared_ptr<runtime::native::NativeRuntime>& runtime() const noexcept {return runtime_;}
private:
    NativeUnitEventQueries()=default;
    std::shared_ptr<runtime::native::NativeRuntime> runtime_;
    bool retired_{};
};
}
