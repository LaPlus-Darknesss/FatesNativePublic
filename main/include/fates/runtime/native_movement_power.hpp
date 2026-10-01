#pragma once
#include "fates/runtime/native_movement_rules.hpp"
namespace fates::runtime::native {
enum class MovementPowerStatus : std::uint8_t {
    Ok,InvalidUnit,MissingDefinition,UnboundAdjustment,StaleAdjustment,AlreadyBound,
    UnboundPenalty,UnboundEnhance,MalformedPair,MissingSkill,MissingCategory,RevisionExhausted,Ignored
};
struct MovementPowerServices {
    virtual ~MovementPowerServices()=default;
    virtual std::optional<std::int8_t> Adjustment()=0;
    virtual std::optional<std::int8_t> Penalty()=0;
    virtual std::optional<bool> EnhanceFlag(unsigned byte,std::uint8_t mask)=0;
    // Selected pointer may be absent: a known null pointer contributes zero.
    virtual std::optional<int> SelectedPartnerMovement()=0;
    virtual std::optional<bool> MovePlusOneSkill()=0;
    virtual std::optional<bool> AmphibiousSkill()=0;
    virtual std::optional<std::uint16_t> Category()=0;
};
// Cached GetMovePower. Repeated category/flag reads and lazy order match original.
std::optional<int> MovementPowerExact(std::uint8_t job_base,std::uint32_t public_flags,
    bool enhanced,int enhance_type,std::uint16_t shooter,std::uint16_t flying,MovementPowerServices&);
struct MovementPowerResult {MovementPowerStatus status{MovementPowerStatus::Ok};int value{};};
MovementPowerResult ProjectCurrentMovementPower(const NativeRuntime&,const UnitState&,bool enhanced=true,int enhance_type=0);
// One-time carried-state binding for Unit+12C; not a constructor or save parser.
MovementPowerStatus RestoreMovementAdjustment(NativeRuntime&,std::uint16_t slot,std::int8_t adjustment);
// Original event callback's key1..250 / force9 no-op gate and byte-wrapping writer.
// Does not require a prior binding because this writer supplies the entire field.
MovementPowerStatus SetCurrentMovementPower(NativeRuntime&,int unit_key,int requested);
}
