#pragma once
class ProcInst;
class Stream;
class Unit;

namespace map {

// Placement/deployment-data owner ("Dispos" is exact retail terminology).
// File/archive ownership and serialized identities are retained semantically;
// archive internals and packet storage remain opaque until their layouts are
// independently closed.
class Dispos {
public:
    struct Header {};
    struct Data {
        struct Assign {};
        void CreateSortie();
        bool CalculateImpl(unsigned int index, Unit* unit, const Assign* assign);
        Unit* GetUnit(unsigned int index);
        void UnitMove(Unit* unit, int x, int y, int direction);
        int Calculate(unsigned int index, const Data* source);
        bool IsEnable(unsigned int index) const;
    };

    static void Initialize();
    void Deserialize(Stream* stream);
    void Serialize(Stream* stream) const;
    void CreateProcess(ProcInst* parent, const char* label, unsigned int flags);
    static bool IsWaitProcess();
    static Dispos* Get();
    void Free();
    bool Load(const char* chapterOrDisposName);
    static void Finalize();
    bool Calculate(Header* header, unsigned int flags);
};

} // namespace map
