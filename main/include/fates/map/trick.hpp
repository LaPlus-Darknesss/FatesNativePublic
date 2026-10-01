#pragma once
#include <cstdint>
class ProcInst;
class Stream;
class Unit;

namespace map {
class Trick {
public:
    static void Initialize();
    static void Deserialize(Stream* stream);
    static Trick* Get();
    void Setup();
    void Regist(bool recreate);
    void Cleanup();
    static void Finalize();
    void Unregist();
    void Serialize(Stream* stream) const;
};

namespace trick {
// Numeric values are retail authority; semantic labels are intentionally held
// until callers/data corroborate every entry.
enum class Type : int {};

class Data {
public:
    Data(Data* next, int x, int y, int arg3, int arg4, Type type,
         int arg6, int arg7, int arg8, int arg9, int arg10, int arg11,
         int arg12, int arg13, const char* label, unsigned char flags);

    void EnableDone(ProcInst* proc);
    void MutationEffect(ProcInst* proc, const char* effectName, int x, int y);
    void Done(ProcInst* proc);
    void Ruin(ProcInst* proc);
    void Enable(ProcInst* proc);
    void Cleanup();
    void Setup();
    void Mutation(ProcInst* proc);

    bool IsBreakable() const;
    bool IsCannonUse(const ::Unit* unit) const;
    bool IsShowDeploy() const;
    void GetFocusPoint(int* x, int* y) const;
    void GetAccessForCannon(int* x, int* y) const;
    void GetUnitDirectionForCannon(int* x, int* y) const;
    bool IsDestroyTargetCastleOffense() const;
    void GetHelp() const;
    bool IsAccess(int x, int y) const;
    bool IsCannon() const;
    void SetDeploy() const;
};

class Enumerator {
public:
    enum class Search : int {};
    Enumerator(Search search, int x, int y, Type type, unsigned char flags);
    virtual ~Enumerator();
    virtual bool IsExclusion(const Data* data);
    void Enumerate();
};
class EnumeratorInfo : public Enumerator { public: using Enumerator::Enumerator; bool IsExclusion(const Data*) override; };
class EnumeratorVisit : public Enumerator { public: using Enumerator::Enumerator; bool IsExclusion(const Data*) override; };
class EnumeratorCannon : public Enumerator { public: using Enumerator::Enumerator; bool IsExclusion(const Data*) override; };
class EnumeratorDeploy : public Enumerator { public: using Enumerator::Enumerator; bool IsExclusion(const Data*) override; };
class EnumeratorEnhance : public Enumerator { public: using Enumerator::Enumerator; bool IsExclusion(const Data*) override; };
class EnumeratorInformal : public Enumerator { public: using Enumerator::Enumerator; bool IsExclusion(const Data*) override; };
class EnumeratorMaterial : public Enumerator { public: using Enumerator::Enumerator; bool IsExclusion(const Data*) override; };
class EnumeratorBreakable : public Enumerator { public: using Enumerator::Enumerator; bool IsExclusion(const Data*) override; };
class EnumeratorCastleDestroyed : public Enumerator { public: using Enumerator::Enumerator; bool IsExclusion(const Data*) override; };

class FocusCalculator { public: explicit FocusCalculator(const char* label); };
class PositionEnumerator {
public:
    explicit PositionEnumerator(const char* label);
    ~PositionEnumerator();
};

} // namespace trick
} // namespace map
