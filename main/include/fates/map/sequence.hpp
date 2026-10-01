#pragma once
#include <cstdint>

class ProcInst;
class Unit;

namespace map {
namespace trick { class Data; }

class Sequence {
public:
    enum class Label : int {};
    static Sequence* GetInstance();
    static void Jump(Label label);
    static void Create(ProcInst* parent);
    static void Resume(ProcInst* parent);
};

class SequenceHuman {
public:
    enum class Label : int {};
    static SequenceHuman* GetInstance();
    static void SetJobIntroTrigger();
    static void Jump(Label label);
    static void Create(ProcInst* parent);
    static void SetCannon(const trick::Data* cannon);
};

class SequenceAI {
public:
    static void InitializeThread();
    static void FinalizeThread();
    static void Create(ProcInst* parent);
};

class SequenceMind {
public:
    static void CreateTalk(ProcInst* parent);
    static void CreateWarp(ProcInst* parent);
    static void CreateItemUse(ProcInst* parent);
    static void CreateDoubleOn(ProcInst* parent);
    static void CreateDoubleOff(ProcInst* parent);
    static void CreateDoubleTrade(ProcInst* parent);
    static void CreateDoubleChange(ProcInst* parent);
    static void Create(ProcInst* parent);
};

class SequenceDance { public: static void Create(ProcInst* parent); };
class SequenceFixed {
public:
    static void Create(ProcInst* parent);
    static void Create2(ProcInst* parent);
};
class SequenceItem { public: static void Create(ProcInst* parent); };
class SequenceLink { public: static void Create(ProcInst* parent); };
class SequenceCannon {
public:
    static void CreateForAuto(ProcInst* parent, Unit* unit, int x, int y);
    static void Create(ProcInst* parent);
};
class SequencePhoenix { public: static void Create(ProcInst* parent); };
class SequenceWinRule { public: static void Create(ProcInst* parent); };
class SequenceInformal {
public:
    static bool Check(int x, int y);
    static void Create(ProcInst* parent, Unit* unit, int x, int y);
};
class SequenceTurnEffect { public: static void Create(ProcInst* parent); };
class SequenceTargetSelect { public: static void Create(ProcInst* parent); };
class SequenceTemporarySave { public: static void Create(ProcInst* parent); };

class SequenceCompleteEffect {
public:
    struct Mvp { std::uint32_t first{}; std::uint32_t second{}; };
    static void Create(ProcInst* parent, const Mvp* mvp);
};

} // namespace map
