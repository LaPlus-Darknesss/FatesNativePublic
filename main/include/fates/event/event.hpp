#pragma once
#include <cstddef>
#include <cstdint>
class ProcInst; class Unit; class Person; class Random;
namespace cmvm { class CmFunction; class CmContext; }
namespace event {
struct Arg;
namespace ErrorLevel { enum Type : int {}; }
namespace Function { enum Type : int {}; }
struct CommandBinding { const char* command; std::uint32_t callbackAddress; const char* callbackSymbol; };
const CommandBinding* GetCommandBindings(); std::size_t GetCommandBindingCount();
bool ScriptLoad(const char* path, ErrorLevel::Type level); void ScriptFree(const char* path, ErrorLevel::Type level);
bool ScriptLoadRoute(const char* path, ErrorLevel::Type level); void ScriptFreeRoute(const char* path, ErrorLevel::Type level);
ProcInst* GetInstance(); void PopInstance(ProcInst*); ProcInst* PushInstance();
void InstantCall(Function::Type type); int InstantCall(const char* functionName); bool IsExistCall(const char* functionName); void Call(ProcInst*, const char* functionName);
Unit* GetUnit(); Unit* GetUnit2(); void SetUnit(const Unit*); void SetUnit2(const Unit*);
void ConsistencyCheck(Function::Type type); void InitializeCommand();
namespace detail {
class ProcEventFacade { public: bool CreateBind(ProcInst*,Function::Type,bool (*)(cmvm::CmFunction*,const Arg*),const Arg*); bool CreateBind(ProcInst*,cmvm::CmFunction*); void Persistent(); void SkipEnable(); void SkipEscape(); void SkipRollback(); void SetNextFunction(); bool IsSearchFunction(); void Tick(); void FadeEnd(); void DestroyOwnedState(); bool IsSkipWait() const; };
}
}
