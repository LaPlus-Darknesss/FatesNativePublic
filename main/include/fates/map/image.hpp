#pragma once
class Stream;
class Unit;
namespace map::trick { class EnumeratorCannon; }

namespace map {

// Tactical range/selection image state. Retail maintains separate talk, unit,
// and danger overlays. Their exact bit-image storage is not exposed as a host
// layout yet; the API records the semantic producer/consumer boundary.
class Image {
public:
    struct Talk {
        static bool IsTalk(const ::Unit* source, const ::Unit* target);
        static void Update(const ::Unit* source);
    };
    struct Unit {
        static void Add(const ::Unit* unit, bool selected, int x, int y);
        static void Delete(const ::Unit* unit, int x, int y);
        static void Update();
        static const ::Unit* GetUnit(int x, int y);
    };
    struct Danger {
        static void Add(const ::Unit* unit, const map::trick::EnumeratorCannon* cannonEnumerator);
        static void Update();
    };

    static void Initialize();
    static void Deserialize(Stream* stream);
    static void Serialize(Stream* stream);
    static void Update(bool forceRefresh);
    static void Finalize();
};

} // namespace map
