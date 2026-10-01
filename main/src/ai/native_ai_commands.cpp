#include "fates/ai/native_ai_commands.hpp"
#include "fates/ai/native_ai_semantics.hpp"

namespace fates::ai::native {
bool IsAiArgument(std::int16_t value) noexcept {return value>=-7&&value<=-4;}
std::int16_t ResolveAiAttackArgument(std::int16_t value,const std::array<std::int16_t,4>& args) noexcept {
    return IsAiArgument(value)?args[static_cast<unsigned>(-value-4)]:value;
}
AiCrossfireResult EvaluateCrossfireCommandsExact(
    std::span<const runtime::native::AiCommandDefinition> commands,
    std::optional<std::uint8_t> activity,const std::array<std::int16_t,4>& args,
    const AiCrossfireTarget& target,const AiCommandIdentityLookup& identities) {
    AiCrossfireResult out{};
    for(const auto& command:commands) {
        if(command.kind==0)return out;
        ++out.visited;
        if(!activity){out.status=AiCrossfireStatus::MissingActivity;return out;}
        if(!IsActiveCommand(command.activity_selector,*activity)||command.kind!=3||command.subtype>=12)continue;
        const auto a=ResolveAiAttackArgument(command.arguments[0],args);
        const auto b=ResolveAiAttackArgument(command.arguments[1],args);
        out.argument_substitutions+=unsigned(IsAiArgument(command.arguments[0]))+unsigned(IsAiArgument(command.arguments[1]));
        if(a==-3||b==-3)continue;
        ++out.target_queries;bool allowed=true;
        switch(command.subtype) {
        case 3:
            if(!target.private_skills){out.status=AiCrossfireStatus::MissingTargetFacts;return out;}
            // SPID_主人公, original labeled PrivateSkill index 1 (distinct from Leader).
            allowed=(*target.private_skills&2u)!=0;break;
        case 4:case 5: {
            const auto person=identities.Person(static_cast<std::uint16_t>(a));
            if(!person){out.status=AiCrossfireStatus::MissingIdentityLookup;return out;}
            allowed=(*person==target.person)==(command.subtype==4);break;
        }
        case 6:
            if(!target.band){out.status=AiCrossfireStatus::MissingTargetFacts;return out;}
            allowed=int(*target.band)!=int(a);break;
        case 7:case 8: {
            const auto job=identities.Job(static_cast<std::uint16_t>(a));
            if(!job){out.status=AiCrossfireStatus::MissingIdentityLookup;return out;}
            allowed=*job==target.job;break;
        }
        case 9:allowed=int(target.force)==int(a);break;
        default:break;
        }
        if(allowed){out.eligible=true;return out;}
    }
    out.status=AiCrossfireStatus::UnterminatedDeclaration;return out;
}
namespace {
struct NativeIdentities final : AiCommandIdentityLookup {
    const runtime::native::DefinitionStore& definitions;
    explicit NativeIdentities(const runtime::native::DefinitionStore& d):definitions(d){}
    std::optional<std::uint16_t> Person(std::uint16_t id) const override {
        const auto* p=definitions.FindPerson(id);return p?std::optional(p->id):std::nullopt;
    }
    std::optional<std::uint16_t> Job(std::uint16_t id) const override {
        const auto* p=definitions.FindJob(id);return p?std::optional(p->id):std::nullopt;
    }
};
}
AiCrossfireResult HasActiveAttackCommandForCrossfire(const runtime::native::NativeRuntime& r,
    std::uint16_t actor_slot,std::uint16_t target_slot) {
    using S=AiCrossfireStatus;
    if(actor_slot>=r.game.units.size()||target_slot>=r.game.units.size())return {S::InvalidUnit};
    const auto& actor=r.game.units[actor_slot];const auto& target=r.game.units[target_slot];
    if(!actor.occupied||!target.occupied||target.force_type>=9)return {S::InvalidUnit};
    if(!actor.ai.configured)return {S::MissingDeclaration};
    const auto* d=r.definitions.FindAiDeclaration(2,actor.ai.attack_id);
    if(!d)return {S::MissingDeclaration};
    const auto* p=r.definitions.FindPerson(target.person_id);const auto* j=r.definitions.FindJob(target.job_id);
    // Target identities must be actual registered semantic definitions. This also
    // prevents a stale or missing Person from being treated as a valid ID match.
    if(!p||!j)return {S::MissingTargetFacts};
    std::uint64_t bits=0;
    for(unsigned i=0;i<8;++i)bits|=std::uint64_t(target.private_skill_bits[i]|p->bitflags[i]|j->bitflags[i])<<(8*i);
    return EvaluateCrossfireCommandsExact(d->commands,actor.ai_activity,actor.ai.attack_args,
        {target.person_id,target.job_id,target.force_type,target.ai_band,bits},NativeIdentities(r.definitions));
}
} // namespace fates::ai::native
