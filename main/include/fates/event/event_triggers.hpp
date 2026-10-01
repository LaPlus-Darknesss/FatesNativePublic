#pragma once
#include "fates/event/event.hpp"
namespace map::trick { enum Type : int; }
namespace event {
#define DECL_SIMPLE(Name) namespace Name { void Trigger(ProcInst*); bool Inspector(cmvm::CmFunction*,const Arg*); }
DECL_SIMPLE(BattleInfo) DECL_SIMPLE(MapSetting) DECL_SIMPLE(TurnTerrain) DECL_SIMPLE(UnitCommand) DECL_SIMPLE(MapSettingResume) DECL_SIMPLE(MapSettingEncount) DECL_SIMPLE(AfterSortie) DECL_SIMPLE(Turn) DECL_SIMPLE(TurnAfter) DECL_SIMPLE(Reinforce)
#undef DECL_SIMPLE
namespace BattleTalk { cmvm::CmFunction* GetFunction(const Arg*); bool Check(const Unit*,const Unit*); void Trigger(ProcInst*,const Unit*,const Unit*); }
namespace InstantDie { void Trigger(ProcInst*,const Unit*); bool Inspector(cmvm::CmFunction*,const Arg*); }
namespace InstantBattle { cmvm::CmFunction* GetFunction(const Arg*); void Trigger(ProcInst*,const Unit*,const Unit*); }
namespace Die { cmvm::CmFunction* GetFunction(Function::Type,const Arg*); bool Check(const Unit*,const Unit*); void Trigger(ProcInst*,const Unit*,const Unit*); }
namespace Area { void Trigger(ProcInst*,int,int); bool Inspector(cmvm::CmFunction*,const Arg*); }
namespace Pick { void Trigger(ProcInst*,const Unit*); bool Inspector(cmvm::CmFunction*,const Arg*); }
namespace Poke { bool Check(int,int,map::trick::Type); void Trigger(ProcInst*,int,int,map::trick::Type); bool Inspector(cmvm::CmFunction*,const Arg*); }
namespace Talk { bool Check(const Unit*,const Unit*); void Trigger(ProcInst*,const Unit*,const Unit*); bool Inspector(cmvm::CmFunction*,const Arg*); }
namespace Escape { void Trigger(ProcInst*,const Unit*); bool Inspector(cmvm::CmFunction*,const Arg*); }
namespace Ending { void Trigger(ProcInst*); } namespace MapFree {void Trigger(ProcInst*);} namespace MapLoad {void Trigger(ProcInst*);} namespace Opening {void Trigger(ProcInst*);} namespace Complete {void Trigger(ProcInst*);} namespace GameOver {void Trigger(ProcInst*);}
}
