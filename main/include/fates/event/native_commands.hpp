#pragma once
#include <cstddef>
#include <cstdint>
#include <string_view>

namespace cmvm { class CmContext; }

namespace fates::event::native {

struct TypedEventRuntime;

using NativeWord = std::intptr_t;

enum class CommandFamily : std::uint8_t {
    Core,
    ChapterUi,
    UnitForce,
    AiMap,
    InventorySocial,
    PresentationField,
    Misc,
};

struct CommandSpec {
    const char* command;
    std::uint32_t retail_address;
    const char* retail_signature;
    CommandFamily family;
};

struct NativeCommandRuntime {
    using InvokeFn = NativeWord (*)(
        void* user,
        const CommandSpec& spec,
        ::cmvm::CmContext* context,
        const NativeWord* args,
        std::size_t argc
    );
    void* user = nullptr;
    InvokeFn invoke = nullptr;
    // Optional typed gameplay surface. Existing {user, invoke} aggregate construction remains valid.
    TypedEventRuntime* typed = nullptr;
};

const CommandSpec* GetCommandSpecs();
std::size_t GetCommandSpecCount();
const CommandSpec* FindCommandSpec(std::string_view command);
NativeWord InvokeNative(
    NativeCommandRuntime& runtime,
    std::string_view command,
    ::cmvm::CmContext* context,
    const NativeWord* args,
    std::size_t argc
);

// These source-facing entry points correspond one-for-one with the 382
// previously unowned ev::* native callbacks recovered from retail.
// Exact retail signatures remain in CommandSpec/evidence; this generic
// word-level ABI deliberately avoids inventing host class layouts.
NativeWord Warning(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord ArgsGetInt(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord ArgsGetString(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord TimeGetSpeed(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord TimeGetConfigSpeed(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord TimeMSecToFrame(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord TimeFrameToMSec(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord FlagEntryGlobal(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord FlagEntry(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord FlagGet(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord FlagSet(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord FlagClr(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord VariableEntryGlobal(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord VariableEntry(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord VariableGet(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord VariableSet(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord VariableAdd(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord VariableInc(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord VariableDec(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord RandomGetGame(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord RandomGetSystem(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord SkipTrigger(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord SkipEscape(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord SkipEnable(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord SkipDisable(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord SkipIsActive(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord SkipIsDisable(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord FadeIn(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord FadeOut(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord FadeIsWait(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord Talk(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord TalkNoShadowFrame(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord Dialog1(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord Dialog2(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord Dialog3(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord Dialog4(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord Dialog5(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord Dialog6(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord Dialog7(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord Dialog8(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord DialogYesNo(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord DialogGetResult(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord RouteBranchDialog(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord RouteBranchBegin(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord RouteBranchEnd(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord ChapterGetName(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord ChapterGetEncountName(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord ChapterGetVersusName(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord ChapterIsCID(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord ChapterSetWinRuleBreakdown(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord ChapterSetWinRuleDestroyBoss(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord ChapterSetWinRuleEscape(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord ChapterSetWinRuleEscapeHero(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord ChapterSetWinRuleEnemyNumberLessThanOrEqualTo(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord ChapterSetWinRuleLimitTurn(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord ChapterSetWinRuleMessageIndex(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord ChapterShowWinRuleTelopImpl(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord ChapterGetWinLoseResult(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord ChapterSetWin(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord ChapterSetLose(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord ChapterSetTemporaryCasual(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord ChapterSetContinueAfterGameOver(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord ChapterSetSkipChapterSaveCastle(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord ChapterSetSkipSortie(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord ChapterSetInvalidSortieSaveBack(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord ChapterSetProhibitWarp(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord ChapterSetHideSystemMenu(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord ChapterSetHideProperty(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord ChapterSetHideTrade(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord ChapterSetHideDouble(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord ChapterSetHideAI(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord ChapterSetHideHpGauge(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord ChapterSetExpMode(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord ChapterSetInvalidPhoenix(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord ChapterSetInvalidGainDragonVein(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord ChapterSetInvalidGainReliance(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord ChapterSetInvalidTransporter(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord ChapterSetInvalidRecordKill(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord ChapterSetInvalidRecordDead(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord ChapterSetInvalidFixedGain(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord ChapterSetValiedSurrender(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord ChapterAppear(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord ChapterIsCleared(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord ChapterIsCastleOffense(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord ChapterIsCastleSelfTest(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord ChapterGetTrickDragon(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord ChapterSetTrickDragon(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord ChapterSetNotReturnMap(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord ChapterSetEscapeEffectWarp(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord ChapterSetResistInterferenceRod(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord MessLoad(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord MessFree(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord MessSetArgumentByMID(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord MessSetArgumentByValue(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord PersonLoad(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord PersonFree(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord TerrainLoad(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord TerrainLoadScript(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord TerrainLoadCastle(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord TerrainFree(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord TerrainGetW(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord TerrainGetH(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord TerrainIsOutOfPlayArea(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord TerrainDrawBegin(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord TerrainDrawEnd(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord DisposLoad(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord DisposFree(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord Dispos(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord DisposIsWait(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord ForceGetActive(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord ForceUnitGetFirst(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord ForceUnitGetNext(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord ForceUnitGetCount(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord ForceUnitGetCountArea(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord ForceUnitTransfer(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord ForceUnitTransferExchange(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord ForceUnitDelete(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord ForceUnitExpulsion(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord UnitGetByPID(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord UnitGetByPIDForce(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord UnitGetByPosition(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord UnitTransferByIndex(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord UnitDeleteByIndex(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord UnitLostByIndex(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord UnitExpulsionByIndex(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord UnitIsAlive(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord UnitIsDoubleParent(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord UnitIsDoubleChild(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord UnitSetPID(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord UnitIsPID(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord UnitIsPIDWithGuest(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord UnitIsResidentFirstPID(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord UnitIsFatherPID(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord UnitIsMotherPID(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord UnitSetJID(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord UnitIsJID(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord UnitIsHighJob(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord UnitIsJCID(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord UnitSetBID(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord UnitIsBID(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord UnitGetForce(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord UnitGetX(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord UnitGetY(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord UnitSetPosition(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord UnitSetPositionFreeSpace(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord UnitMovePosition(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord UnitMoveSlide(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord UnitSetAlpha(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord UnitMoveAlphaDeadOut(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord UnitMoveAlphaWarpIn(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord UnitMoveAlphaWarpOut(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord UnitIsMoveWait(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord UnitSetDontAttack(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord UnitSetDontAttackForce(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord UnitGetLevel(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord UnitGetInternalLevel(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord UnitGetHP(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord UnitGetMHP(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord UnitSetHP(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord UnitGetCapability(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord UnitSetCapability(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord UnitGetMovePower(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord UnitSetMovePower(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord UnitGetWeaponLevel(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord UnitGetMarrigeUnit(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord UnitGetRecordKill(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord UnitItemIsExist(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord UnitItemAddDirect(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord UnitItemPutOff(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord UnitItemPutOffAll(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord UnitItemToTransporter(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord UnitItemDisposalFor006(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord UnitItemEquip(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord UnitItemEquip2(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord UnitItemClearEquip(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord UnitItemClearDrop(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord UnitItemSetEndurance(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord UnitItemSetInvalidRefine(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord UnitShowDanger(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord UnitShowWholeDanger(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord UnitSetPrivateSkill(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord UnitClrPrivateSkill(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord UnitTstPrivateSkill(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord UnitCanDragonVein(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord UnitIsEquipSkill(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord UnitAddEquipSkill(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord UnitDoubleOn(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord UnitDoubleOff(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord UnitBackup(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord UnitRestore(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord UnitBackupDiscard(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord UnitHpDamage(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord UnitHpHeal(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord UnitIsHpEffectWait(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord UnitSetEnhance(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord UnitClrEnhance(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord UnitTstEnhance(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord UnitGetWeakness(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord UnitAddWeakness(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord UnitMergeWeakness(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord UnitCreateClone(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord UnitIsClone(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord UnitSetNeverSortie(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord UnitRecreate(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord UnitSetPhantom(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord UnitIsPhantom(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord UnitTstStatus(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord UnitSetStatus(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord UnitClrStatus(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord UnitIsAbleToAttack(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord UnitIsSleep(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord UnitSetMotion(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord UnitGetRelianceLevel(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord UnitSetInvalidEternal(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord UnitSetInvalidUseDoping(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord UnitSetInvalidUseClassChange(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord UnitShine(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord UnitSetDummyIcon(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord AiSetSequence(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord AiTstFlag(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord AiSetFlag(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord AiClrFlag(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord AiSetBattleRate(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord AiSetHealRate(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord AiGetHealRateA(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord AiGetHealRateB(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord AiSetActive(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord AiGetActive(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord AiGetBand(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord AiSetMoveLimit(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord AiGetDisposX(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord AiGetDisposY(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord AiSetDisposPosition(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord AiGetPriority(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord AiSetPriority(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord TransporterItemIsExist(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord TransporterItemPutOff(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord TransporterItemSetInvalidRefine(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord TerrainAttributeSetTID(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord TerrainAttributeIsTID(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord TerrainAttributeIsTIDWithEdge(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord TerrainAttributeIsFlag(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord TerrainAttributeRemoveObstacle(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord TerrainAttributeUpdatePosition(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord TerrainAttributeUpdateArea(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord TrickGetForce(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord TrickSetForce(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord TrickSet(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord TrickStringSet(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord TrickTst(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord TrickTryTst(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord TrickGetHp(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord TrickSetHp(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord TrickGetMaxHp(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord TrickGetDragonAccessX(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord TrickGetDragonAccessY(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord TrickSetDragonFocus(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord GimmickMedicine(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord GimmickMedicineString(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord GimmickWindBlow(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord MindGetForce(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord MindGetUnit(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord MindGetX(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord MindGetY(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord MindGetMind(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord MindGetTargetUnit(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord MindGetTradeUnit(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord RouteGet(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord RouteSet(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord RouteIsBought(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord ContentsGetPackage(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord DifficultyGet(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord DifficultyIsCasual(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord DifficultyIsPhoenix(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord ScenarioRankGet(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord TurnGet(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord ItemGain(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord ItemGainNoSound(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord ItemGainSilent(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord ItemReplaceForYatonokami(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord ItemGainMessageOnly(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord ItemGainMessageOnlyNoSound(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord ItemGetKind(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord ItemGetOrigin(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord ItemGetRandom(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord GoldGain(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord GoldGain2(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord GoldGainSilent(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord GoldGet(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord AccessoryGain(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord AccessoryGainSilent(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord EffectPlay3D(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord EffectPlayEternal3D(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord EffectPlayUnit(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord EffectRotate(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord EffectFadeOut(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord EffectDelete(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord EffectIsPreload(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord EffectIsPlaying(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord EffectHideTrick(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord EffectShowTrick(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord TelopPlay(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord TelopPlayDirect(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord TelopIsWait(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord TutorialIsEnable(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord TutorialShow(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord TutorialGetFlag(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord TutorialSetFlag(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord TutorialSetFlagAll(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord TutorialDialog(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord TutorialDialogMustShow(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord TutorialIsAbleToAttack(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord TutorialIsAbleToTalkForPickEvent(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord MapSuspend(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord MapResume(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord MapImageUpdate(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord MoviePlayImpl(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord MovieStop(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord MovieIsExist(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord MovieIsPaused(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord MovieIsSkipped(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord MovieEnablePauseAtEnd(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord MovieDisablePauseAtEnd(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord Battle(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord JoinGuest(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord CardEntry(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord CardIsExist(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord BGMPlay(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord BGMStop(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord RBGMPlay(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord RBGMStop(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord RBGMEffect(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord BGMVolume(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord EnableBGMCommandPG(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord DisableBGMCommandPG(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord BMapBGMChnageFlagOn(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord BMapBGMResume(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord SEPlay(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord SEPlay3D(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord LSEPlay(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord LSEStop(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord LSEVolume(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord Voice(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord EnvSEOff(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord EnvSEOn(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord VoiceArchiveLoad(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord VoiceArchiveFree(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord MiracleTelop(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord MiracleShoot(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord MiracleShootXY(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord MiracleGetShootX(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord MiracleGetShootY(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord GMapMove(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord FieldObjectPlayState(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord FieldObjectSetState(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord FieldObjectSetVisible(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord FieldObjectSetEscape(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord FieldObjectCreate(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord FieldObjectDelete(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord FieldObjectMove(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord FieldObjectMoveEx(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord FieldObjectWarp(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord FieldObjectFadeOut(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord FieldObjectPlayAnime(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord FieldObjectPlayEffect(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord UnitRescue(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord UnitRescueAll(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord LilithDie(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord EpithetUnlock(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord AnnaMessageSet(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord AnnaMessageClr(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord BandOpen(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord BandClose(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord CrossFade(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord ContentsGetFlag(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord ContentsSetFlag(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord ContentsClrFlag(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord ContentsGetIndex(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord ContentsGetElapse(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord ContentsUpdateTime(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord ComebackCastle(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);
NativeWord AmiiboUpdate(NativeCommandRuntime&, ::cmvm::CmContext*, const NativeWord* args, std::size_t argc);

} // namespace fates::event::native
