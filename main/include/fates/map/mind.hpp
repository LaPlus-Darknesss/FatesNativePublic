#pragma once

class Unit;

namespace map {

// Tier A. PROVEN: retail owns one global Mind object and uses it as the current
// tactical decision/selection context. The exact storage layout remains OPAQUE.
class Mind {
public:
    static void Initialize();
    static Mind* Get();
    static void Finalize();

    void Continuous();
    void ResetTarget();
    void DoubleChange(const Unit* unit);
    void Reset(const Unit* unit);
    void ResetMind();

    const Unit* GetTradeUnit() const;
    const Unit* GetTargetUnit() const;
    const Unit* GetDoubleTradeUnit() const;
    const Unit* GetUnit() const;
};

} // namespace map
