#pragma once
#include <cstdint>
#include "fates/game/gameplay_types.hpp"

class Item;
namespace unit {
class Item {
public:
    Item();
    Item& operator=(const Item& other);

    const ::Item* ToData() const;
    void DeserializeIfNotExistKeep(Stream* stream);
    static bool SortCompare(const Item* lhs, const Item* rhs, void* context);
    void SetEndurance(int endurance);
    void SetRefineRank(int refineRank);
    void DeserializeIfNotExistDelete(Stream* stream);
    void New(const ::Item* item);
    void New(const char* identifier);
    void New(std::uint16_t itemId);
    void Clear();
    bool Expend();
    void Replace(std::uint16_t itemId);
    void Serialize(Stream* stream) const;

    int GetCritical() const;
    bool IsIntegrate(const Item* other) const;
    int GetEndurance() const;
    int GetSellPrice() const;
    bool IsShopRefine(const Item* other) const;
    bool IsShopRefine() const;
    int GetRefineRank() const;
    int GetHit() const;
    const wchar_t* GetName() const;
    std::uint64_t GetSort() const;
    bool IsEqual(const Item* other) const;
    int GetPower() const;
    bool IsExpend() const;

    bool IsEmpty() const { return itemId_ == 0; }
    std::uint16_t ItemId() const { return itemId_; }

private:
    // Retail evidence proves this class is exactly two 16-bit words. The high
    // six bits at state[8..13] are endurance for non-weapons and refine rank
    // for weapons; state[0..6] participates in variant/refine identity.
    std::uint16_t itemId_{};
    std::uint16_t state_{};
};
static_assert(sizeof(Item) == 4);
}
