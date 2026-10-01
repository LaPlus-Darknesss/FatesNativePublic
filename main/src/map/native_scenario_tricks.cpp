#include "fates/map/native_scenario_tricks.hpp"
#include "fates/map/native_terrain_semantics.hpp"
#include "fates/support/native_local_support.hpp"
#include <algorithm>
#include <set>
#include <string>
namespace fates::map::native {
namespace rn=fates::runtime::native;
namespace {
std::array<std::uint32_t,6> Bounds(const rn::TerrainMapDefinition& t) {
    return {t.width,t.height,t.min_x,t.min_y,t.max_x,t.max_y};
}
bool CurrentUserModeAllowed(const rn::NativeRuntime& r) {
    return (r.game.campaign.game_user_flags&0x22u)==0;
}
}
std::array<std::uint8_t,15> ConstructInitialTrickBytes(
    const std::array<std::uint32_t,13>& a,std::uint8_t flags) noexcept {
    std::array<std::uint8_t,15> out{};
    for(unsigned i=0;i<6;++i)out[i]=static_cast<std::uint8_t>(a[i]);
    out[6]=static_cast<std::uint8_t>(a[5]);
    for(unsigned i=6;i<13;++i)out[i+1]=static_cast<std::uint8_t>(a[i]);
    out[14]=flags;return out;
}
ScenarioTrickResult RegisterInitialScenarioScripts(rn::NativeRuntime& r,NativeScenarioKind kind,
    std::uint8_t difficulty,std::span<const ScenarioScriptInput> sources) {
    ScenarioTrickResult result{};
    if(!r.game.map_active || !r.definitions.terrain_map()){result.status=ScenarioTrickStatus::InvalidMap;return result;}
    if(kind!=NativeScenarioKind::OrdinaryCampaign || !rn::GameUserDifficultyMatches(r,difficulty) || !CurrentUserModeAllowed(r)) {
        result.status=ScenarioTrickStatus::InvalidMode;return result;
    }
    if(r.game.scenario_tricks.bound){result.status=ScenarioTrickStatus::AlreadyBound;return result;}
    if(sources.empty() || sources.size()>64){result.status=ScenarioTrickStatus::InvalidArchiveSet;return result;}
    rn::NativeScenarioTrickState next{};std::set<std::string> names;
    std::vector<std::vector<fates::cmvm::ScriptTrickDeclaration>> parsed;
    for(std::size_t index=0;index<sources.size();++index) {
        const auto& source=sources[index];result.archive_index=static_cast<std::uint16_t>(index);
        if(source.identity.empty() || source.identity.size()>240 || source.identity.find('\0')!=std::string_view::npos ||
           !names.insert(std::string(source.identity)).second){result.status=ScenarioTrickStatus::InvalidArchiveSet;return result;}
        parsed.emplace_back();
        result.archive_status=fates::cmvm::ReadScriptTrickDeclarations(source.original_bytes,parsed.back());
        if(result.archive_status!=fates::cmvm::TrickArchiveStatus::Ok){result.status=ScenarioTrickStatus::InvalidArchive;return result;}
        next.attached_archives.push_back({std::string(source.identity),{source.original_bytes.begin(),source.original_bytes.end()}});
    }
    // Regist enumerates ALL type21 functions across attached archives first,
    // then ALL type22 functions. Every constructed node is prepended.
    for(const unsigned type:{21u,22u})for(std::size_t i=0;i<parsed.size();++i)for(const auto& d:parsed[i]) {
        if(d.event_type!=type)continue;
        auto bytes=ConstructInitialTrickBytes(d.arguments);
        if(bytes[4]==14){result.status=ScenarioTrickStatus::RegistrationSideEffectRequired;result.archive_index=static_cast<std::uint16_t>(i);result.function_index=d.function_index;return result;}
        next.registry.push_back({bytes,static_cast<std::uint16_t>(i),d.function_index,d.event_type,d.label});
        if(next.registry.size()>4096){result.status=ScenarioTrickStatus::InvalidArchiveSet;return result;}
    }
    std::reverse(next.registry.begin(),next.registry.end());
    const auto& map=*r.definitions.terrain_map();
    next.bound=true;next.difficulty=difficulty;next.chapter=r.game.campaign.current_chapter_index;
    next.difficulty_revision=r.game.game_user_difficulty.revision;
    next.phase_revision=r.game.phase.revision;next.user_flags=r.game.campaign.game_user_flags;
    next.control=r.game.phase.situation.control;
    next.map_bounds=Bounds(map);next.terrain_grid=map.grid;
    result.status=ScenarioTrickStatus::Ok;result.declarations=static_cast<std::uint32_t>(next.registry.size());
    for(const auto& d:next.registry)result.mounted_cannons+=unsigned(IsCannon(d.fields[4]));
    r.game.scenario_tricks=std::move(next);return result;
}
void InvalidateScenarioScripts(rn::NativeRuntime& r) noexcept {r.game.scenario_tricks.bound=false;}
bool ScenarioScriptsMatch(const rn::NativeRuntime& r) noexcept {
    const auto& s=r.game.scenario_tricks;
    return s.bound && r.game.map_active && r.definitions.terrain_map() && rn::GameUserDifficultyMatches(r,s.difficulty) && s.difficulty_revision==r.game.game_user_difficulty.revision && CurrentUserModeAllowed(r) &&
        s.chapter==r.game.campaign.current_chapter_index && s.phase_revision==r.game.phase.revision &&
        s.user_flags==r.game.campaign.game_user_flags &&
        s.control==r.game.phase.situation.control && s.map_bounds==Bounds(*r.definitions.terrain_map()) &&
        s.terrain_grid==r.definitions.terrain_map()->grid;
}
bool IntrinsicCannonCategory(std::uint16_t category) noexcept {return (category&0x0800u)!=0;}
ScenarioTrickResult InspectScriptCannonAbsence(const rn::NativeRuntime& r,std::uint16_t slot) {
    ScenarioTrickResult out{};
    if(!r.game.scenario_tricks.bound)return out;
    if(!ScenarioScriptsMatch(r)){out.status=ScenarioTrickStatus::StaleContext;return out;}
    if(slot>=r.game.units.size() || !r.game.units[slot].occupied){out.status=ScenarioTrickStatus::InvalidActor;return out;}
    const auto* j=r.definitions.FindJob(r.game.units[slot].job_id);
    if(!j){out.status=ScenarioTrickStatus::InvalidActor;return out;}
    // GetJobCategory's private-flag edits affect Dragon/Beast only. They do
    // not synthesize or remove Shooter (0x0800). Preserve intrinsic artillery
    // independently of whether the mounted-cannon collection is empty.
    if(IntrinsicCannonCategory(static_cast<std::uint16_t>(j->special_flags[0]|(unsigned(j->special_flags[1])<<8u)))){out.status=ScenarioTrickStatus::IntrinsicCannon;return out;}
    out.declarations=static_cast<std::uint32_t>(r.game.scenario_tricks.registry.size());
    for(const auto& d:r.game.scenario_tricks.registry)out.mounted_cannons+=unsigned(IsCannon(d.fields[4]));
    out.status=out.mounted_cannons?ScenarioTrickStatus::MountedCannonPresent:ScenarioTrickStatus::Ok;
    return out;
}
bool DualAlternativeEarlyRejectedExact(bool no_dual,bool can_dual,std::uint8_t difficulty) noexcept {
    return no_dual || !can_dual || difficulty==0;
}
bool ScenarioDualAlternativeEarlyRejected(const rn::NativeRuntime& r,std::uint16_t slot) noexcept {
    if(!ScenarioScriptsMatch(r) || slot>=r.game.units.size())return false;
    const auto& u=r.game.units[slot];
    const auto* p=r.definitions.FindPerson(u.person_id);const auto* j=r.definitions.FindJob(u.job_id);
    if(!u.occupied || u.force_type>=3 || !p || !j)return false;
    const auto flags=u.private_skill_bits[4]|p->bitflags[4]|j->bitflags[4];
    // CanDual is not synthesized from an unbound Situation flag word here.
    // A private NoDual bit or Normal difficulty is sufficient independently of
    // that intervening gate; otherwise the whole-force alternative stays open.
    return DualAlternativeEarlyRejectedExact((flags&0x10u)!=0,true,r.game.scenario_tricks.difficulty);
}
} // namespace fates::map::native
