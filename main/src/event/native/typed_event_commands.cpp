#include "fates/event/typed_event_commands.hpp"

#include <cstdint>

namespace fates::event::native {
namespace {
constexpr TypedCommandSpec kTypedSpecs[] = {
    {"ev::FlagGet",0X005084ECu,TypedCommandFamily::Core,1},
    {"ev::FlagSet",0X003B0624u,TypedCommandFamily::Core,1},
    {"ev::FlagClr",0X003B05E8u,TypedCommandFamily::Core,1},
    {"ev::VariableGet",0X0050BCB0u,TypedCommandFamily::Core,1},
    {"ev::VariableSet",0X001E7FE8u,TypedCommandFamily::Core,2},
    {"ev::VariableAdd",0X001E7F50u,TypedCommandFamily::Core,2},
    {"ev::VariableInc",0X003A7E78u,TypedCommandFamily::Core,1},
    {"ev::VariableDec",0X003A7E58u,TypedCommandFamily::Core,1},
    {"ev::RandomGetGame",0X003A8BC4u,TypedCommandFamily::Core,0},
    {"ev::RandomGetSystem",0X003AA868u,TypedCommandFamily::Core,0},
    {"ev::ChapterGetName",0X003A92E8u,TypedCommandFamily::Chapter,0},
    {"ev::ChapterIsCID",0X003A7F44u,TypedCommandFamily::Chapter,1},
    {"ev::ChapterGetWinLoseResult",0X003AE6E8u,TypedCommandFamily::Chapter,0},
    {"ev::ChapterSetWin",0X003A8A50u,TypedCommandFamily::Chapter,0},
    {"ev::ChapterSetLose",0X003A9310u,TypedCommandFamily::Chapter,0},
    {"ev::ChapterSetSkipSortie",0X003AD7C8u,TypedCommandFamily::Chapter,1},
    {"ev::ChapterSetProhibitWarp",0X003AE100u,TypedCommandFamily::Chapter,1},
    {"ev::ChapterSetExpMode",0X003AB744u,TypedCommandFamily::Chapter,1},
    {"ev::ChapterSetInvalidPhoenix",0X003AE85Cu,TypedCommandFamily::Chapter,1},
    {"ev::ChapterSetInvalidGainDragonVein",0X003AF1C4u,TypedCommandFamily::Chapter,1},
    {"ev::ChapterSetWinRuleLimitTurn",0X003AED30u,TypedCommandFamily::Chapter,1},
    {"ev::ChapterSetWinRuleEnemyNumberLessThanOrEqualTo",0X003AF354u,TypedCommandFamily::Chapter,1},
    {"ev::DisposLoad",0X0039ED84u,TypedCommandFamily::Dispos,1},
    {"ev::DisposFree",0X0039ED28u,TypedCommandFamily::Dispos,0},
    {"ev::Dispos",0X0039DB4Cu,TypedCommandFamily::Dispos,2},
    {"ev::DisposIsWait",0X0039DFACu,TypedCommandFamily::Dispos,0},
    {"ev::ForceGetActive",0X003A9490u,TypedCommandFamily::Force,0},
    {"ev::ForceUnitGetCount",0X003AB838u,TypedCommandFamily::Force,1},
    {"ev::UnitGetByPID",0X003A8390u,TypedCommandFamily::Unit,1},
    {"ev::UnitIsPID",0X003B0FE4u,TypedCommandFamily::Unit,2},
    {"ev::UnitGetByPosition",0X003ABE80u,TypedCommandFamily::Unit,2},
    {"ev::UnitGetForce",0X003A83C0u,TypedCommandFamily::Unit,1},
    {"ev::UnitGetX",0X003B0AD0u,TypedCommandFamily::Unit,1},
    {"ev::UnitGetY",0X003B0B20u,TypedCommandFamily::Unit,1},
    {"ev::UnitSetPosition",0X003AAD88u,TypedCommandFamily::Unit,3},
    {"ev::UnitMovePosition",0X003AB1A4u,TypedCommandFamily::Unit,4},
    {"ev::UnitIsMoveWait",0X003A9858u,TypedCommandFamily::Unit,0},
    {"ev::UnitIsAlive",0X003A7C80u,TypedCommandFamily::Unit,1},
    {"ev::UnitGetHP",0X003B0ED4u,TypedCommandFamily::Unit,1},
    {"ev::UnitGetMHP",0X003A73D8u,TypedCommandFamily::Unit,1},
    {"ev::UnitSetHP",0X003B105Cu,TypedCommandFamily::Unit,2},
    {"ev::UnitGetMovePower",0X0052D580u,TypedCommandFamily::Unit,2},
    {"ev::UnitSetMovePower",0X003AB694u,TypedCommandFamily::Unit,2},
    {"ev::AiSetSequence",0X003A8934u,TypedCommandFamily::Ai,4},
    {"ev::AiTstFlag",0X003B0C44u,TypedCommandFamily::Ai,2},
    {"ev::AiSetFlag",0X003B0BFCu,TypedCommandFamily::Ai,2},
    {"ev::AiClrFlag",0X003B0B70u,TypedCommandFamily::Ai,2},
    {"ev::AiSetActive",0X003A776Cu,TypedCommandFamily::Ai,2},
    {"ev::AiGetActive",0X003A7728u,TypedCommandFamily::Ai,1},
    {"ev::AiSetMoveLimit",0X003A9204u,TypedCommandFamily::Ai,6},
    {"ev::AiSetPriority",0X003A88F4u,TypedCommandFamily::Ai,2},
};

const char* StringArg(const NativeWord* args, std::size_t index) {
    return reinterpret_cast<const char*>(static_cast<std::uintptr_t>(args[index]));
}
std::int32_t IntArg(const NativeWord* args, std::size_t index) {
    return static_cast<std::int32_t>(args[index]);
}
std::uint32_t UIntArg(const NativeWord* args, std::size_t index) {
    return static_cast<std::uint32_t>(args[index]);
}
bool BoolArg(const NativeWord* args, std::size_t index) { return args[index] != 0; }
std::uint8_t ByteArg(const NativeWord* args, std::size_t index) {
    return static_cast<std::uint8_t>(static_cast<std::uintptr_t>(args[index]) & 0xffu);
}
std::uint16_t HalfArg(const NativeWord* args, std::size_t index) {
    return static_cast<std::uint16_t>(static_cast<std::uintptr_t>(args[index]) & 0xffffu);
}
NativeWord PointerResult(const char* value) {
    return static_cast<NativeWord>(reinterpret_cast<std::uintptr_t>(value));
}
}

const TypedCommandSpec* GetTypedCommandSpecs() { return kTypedSpecs; }
std::size_t GetTypedCommandSpecCount() { return sizeof(kTypedSpecs) / sizeof(kTypedSpecs[0]); }
const TypedCommandSpec* FindTypedCommandSpec(std::string_view command) {
    for (const auto& spec : kTypedSpecs) if (command == spec.command) return &spec;
    return nullptr;
}

bool TryInvokeTyped(TypedEventRuntime& r, std::string_view c, cmvm::CmContext*, const NativeWord* a, std::size_t n, NativeWord& out) {
    const auto* spec = FindTypedCommandSpec(c);
    if (!spec || n != spec->argument_count || (n != 0 && a == nullptr)) return false;
    void* u = r.user;
#define RETV(cb, expr) do { if (!(cb)) return false; out = static_cast<NativeWord>(expr); return true; } while(false)
#define RET0(cb, call) do { if (!(cb)) return false; call; out = 0; return true; } while(false)
    if (c=="ev::FlagGet") RETV(r.core.flag_get, r.core.flag_get(u,StringArg(a,0)) ? 1 : 0);
    if (c=="ev::FlagSet") RET0(r.core.flag_set, r.core.flag_set(u,StringArg(a,0)));
    if (c=="ev::FlagClr") RET0(r.core.flag_clear, r.core.flag_clear(u,StringArg(a,0)));
    if (c=="ev::VariableGet") RETV(r.core.variable_get, r.core.variable_get(u,StringArg(a,0)));
    if (c=="ev::VariableSet") RET0(r.core.variable_set, r.core.variable_set(u,StringArg(a,0),IntArg(a,1)));
    if (c=="ev::VariableAdd") RET0(r.core.variable_add, r.core.variable_add(u,StringArg(a,0),IntArg(a,1)));
    if (c=="ev::VariableInc") RET0(r.core.variable_add, r.core.variable_add(u,StringArg(a,0),1));
    if (c=="ev::VariableDec") RET0(r.core.variable_add, r.core.variable_add(u,StringArg(a,0),-1));
    if (c=="ev::RandomGetGame") RETV(r.core.random_get_game, r.core.random_get_game(u));
    if (c=="ev::RandomGetSystem") RETV(r.core.random_get_system, r.core.random_get_system(u));

    if (c=="ev::ChapterGetName") { if(!r.chapter.get_name) return false; out=PointerResult(r.chapter.get_name(u)); return true; }
    if (c=="ev::ChapterIsCID") RETV(r.chapter.is_cid, r.chapter.is_cid(u,StringArg(a,0)) ? 1 : 0);
    if (c=="ev::ChapterGetWinLoseResult") RETV(r.chapter.get_win_lose_result, r.chapter.get_win_lose_result(u));
    if (c=="ev::ChapterSetWin") RET0(r.chapter.set_win, r.chapter.set_win(u));
    if (c=="ev::ChapterSetLose") RET0(r.chapter.set_lose, r.chapter.set_lose(u));
    if (c=="ev::ChapterSetSkipSortie") RET0(r.chapter.set_skip_sortie, r.chapter.set_skip_sortie(u,BoolArg(a,0)));
    if (c=="ev::ChapterSetProhibitWarp") RET0(r.chapter.set_prohibit_warp, r.chapter.set_prohibit_warp(u,BoolArg(a,0)));
    if (c=="ev::ChapterSetExpMode") RET0(r.chapter.set_exp_mode, r.chapter.set_exp_mode(u,ByteArg(a,0)));
    if (c=="ev::ChapterSetInvalidPhoenix") RET0(r.chapter.set_invalid_phoenix, r.chapter.set_invalid_phoenix(u,BoolArg(a,0)));
    if (c=="ev::ChapterSetInvalidGainDragonVein") RET0(r.chapter.set_invalid_gain_dragon_vein, r.chapter.set_invalid_gain_dragon_vein(u,BoolArg(a,0)));
    if (c=="ev::ChapterSetWinRuleLimitTurn") RET0(r.chapter.set_win_rule_limit_turn, r.chapter.set_win_rule_limit_turn(u,HalfArg(a,0)));
    if (c=="ev::ChapterSetWinRuleEnemyNumberLessThanOrEqualTo") RET0(r.chapter.set_win_rule_enemy_number_le, r.chapter.set_win_rule_enemy_number_le(u,ByteArg(a,0)));

    if (c=="ev::DisposLoad") RETV(r.dispos.load, r.dispos.load(u,StringArg(a,0)) ? 1 : 0);
    if (c=="ev::DisposFree") RET0(r.dispos.free, r.dispos.free(u));
    if (c=="ev::Dispos") RET0(r.dispos.run_group, r.dispos.run_group(u,StringArg(a,0),IntArg(a,1)));
    if (c=="ev::DisposIsWait") RETV(r.dispos.is_wait, r.dispos.is_wait(u) ? 1 : 0);

    if (c=="ev::ForceGetActive") RETV(r.force.get_active, r.force.get_active(u));
    if (c=="ev::ForceUnitGetCount") RETV(r.force.unit_get_count, r.force.unit_get_count(u,IntArg(a,0)));

    if (c=="ev::UnitGetByPID") RETV(r.unit.get_by_pid, r.unit.get_by_pid(u,StringArg(a,0)));
    if (c=="ev::UnitIsPID") RETV(r.unit.is_pid, r.unit.is_pid(u,IntArg(a,0),StringArg(a,1)) ? 1 : 0);
    if (c=="ev::UnitGetByPosition") RETV(r.unit.get_by_position, r.unit.get_by_position(u,IntArg(a,0),IntArg(a,1)));
    if (c=="ev::UnitGetForce") RETV(r.unit.get_force, r.unit.get_force(u,IntArg(a,0)));
    if (c=="ev::UnitGetX") RETV(r.unit.get_x, r.unit.get_x(u,IntArg(a,0)));
    if (c=="ev::UnitGetY") RETV(r.unit.get_y, r.unit.get_y(u,IntArg(a,0)));
    if (c=="ev::UnitSetPosition") RET0(r.unit.set_position, r.unit.set_position(u,IntArg(a,0),IntArg(a,1),IntArg(a,2)));
    if (c=="ev::UnitMovePosition") RET0(r.unit.move_position, r.unit.move_position(u,IntArg(a,0),IntArg(a,1),IntArg(a,2),UIntArg(a,3)));
    if (c=="ev::UnitIsMoveWait") RETV(r.unit.is_move_wait, r.unit.is_move_wait(u) ? 1 : 0);
    if (c=="ev::UnitIsAlive") RETV(r.unit.is_alive, r.unit.is_alive(u,IntArg(a,0)) ? 1 : 0);
    if (c=="ev::UnitGetHP") RETV(r.unit.get_hp, r.unit.get_hp(u,IntArg(a,0)));
    if (c=="ev::UnitGetMHP") RETV(r.unit.get_mhp, r.unit.get_mhp(u,IntArg(a,0)));
    if (c=="ev::UnitSetHP") RET0(r.unit.set_hp, r.unit.set_hp(u,IntArg(a,0),IntArg(a,1)));
    if (c=="ev::UnitGetMovePower") RETV(r.unit.get_move_power, r.unit.get_move_power(u,IntArg(a,0),BoolArg(a,1)));
    if (c=="ev::UnitSetMovePower") RET0(r.unit.set_move_power, r.unit.set_move_power(u,IntArg(a,0),IntArg(a,1)));

    if (c=="ev::AiSetSequence") RET0(r.ai.set_sequence, r.ai.set_sequence(u,IntArg(a,0),ByteArg(a,1),StringArg(a,2),StringArg(a,3)));
    if (c=="ev::AiTstFlag") RETV(r.ai.test_flag, r.ai.test_flag(u,IntArg(a,0),UIntArg(a,1)));
    if (c=="ev::AiSetFlag") RET0(r.ai.set_flag, r.ai.set_flag(u,IntArg(a,0),UIntArg(a,1)));
    if (c=="ev::AiClrFlag") RET0(r.ai.clear_flag, r.ai.clear_flag(u,IntArg(a,0),UIntArg(a,1)));
    if (c=="ev::AiSetActive") RET0(r.ai.set_active, r.ai.set_active(u,IntArg(a,0),ByteArg(a,1)));
    if (c=="ev::AiGetActive") RETV(r.ai.get_active, r.ai.get_active(u,IntArg(a,0)));
    if (c=="ev::AiSetMoveLimit") RET0(r.ai.set_move_limit, r.ai.set_move_limit(u,IntArg(a,0),ByteArg(a,1),ByteArg(a,2),ByteArg(a,3),ByteArg(a,4),ByteArg(a,5)));
    if (c=="ev::AiSetPriority") RET0(r.ai.set_priority, r.ai.set_priority(u,IntArg(a,0),ByteArg(a,1)));
#undef RETV
#undef RET0
    return false;
}

} // namespace fates::event::native
