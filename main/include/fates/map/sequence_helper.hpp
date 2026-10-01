#pragma once
class ProcInst;
class Unit;
namespace unit { class Item; }
namespace map::trick { class Data; }

namespace map::SequenceHelper {
namespace Material { enum class Type : int; }

bool DangerTick();
void MoveCursor(ProcInst* parent, int x, int y);
void MaterialGain(ProcInst* parent, Material::Type type, int amount, int subType);
void MaterialGain(ProcInst* parent, map::trick::Data* trickData);
void CheerFromMind(ProcInst* parent);
bool DangerOneTick();
void GoldGainSilent(int forceType, int amount);
bool HpEffectIsWait();
void ItemGainSilent(Unit* unit, const unit::Item* item);
bool GetCursorTurnFirst(int forceType, int* x, int* y);
void FreeCursorSetMoveImage(const Unit* unit);
void CheerFromCapabilityIndex(ProcInst* parent, Unit* unit, int capabilityIndex);
void ItemChapterLimitedToTransporter(Unit* unit, bool includeEquipped);
bool IsGainItemWrong(const Unit* unit, const unit::Item* item);
void HpHeal(ProcInst* parent, Unit* unit, int amount, bool showEffect, bool wait);
void GoldGain(ProcInst* parent, int forceType, int amount, const char* message, bool wait);
void HpDamage(ProcInst* parent, Unit* unit, int amount, bool showEffect, bool wait);
void ItemGain(ProcInst* parent, Unit* unit, const unit::Item* item, bool showMessage);

namespace detail {
class HpHealProcess { public: void PlayEffect(); void Construct(Unit*,int); void Execute(); };
class GainItemProcess { public: void ShowMessage(); void SkipRollback(); void Branch(); void GainImpl(); void Send(); };
class HpDamageProcess { public: void PlayEffect(); void Execute(); void Construct(Unit*,int); };
class HpEffectProcess {
public:
    void Persistent(); void ExecuteWait(); void HpWindowOpen(); void HpWindowWait();
    void EffectEndWait(); void HpWindowClose(); void HpWindowCreate(); void HpWindowDelete();
    void ClearSkipStatus(); void EffectBeginWait(); void SkipInputEnable(); void SkipInputDisable();
};
class WaitCursorProcess { public: void Tick(); };
class GainMaterialProcess { public: void ShowMessage(); void GainImpl(); };
class GainMaterialFromTrickProcess { public: void GainImpl(); };
class HpEffectCallbackProcess { public: static void Callback(ProcInst*); };
void CheerImpl(ProcInst* parent, Unit* source, Unit* target, int capabilityIndex, bool partner);
}
} // namespace map::SequenceHelper
