#pragma once
#include "fates/detail/effect_runtime.hpp"
#include "fates/io/file_object.hpp"
#include "fates/memory/iallocator.hpp"
class EffectObject : public FileObject {
public:
    EffectObject();
    ~EffectObject() override = default; // deleting-destructor retail entry stays ABI evidence only
    void Setup() override;
    void Cleanup() override;
    IAllocator* GetAllocator() const override;
    bool IsDelay() const override;
    unsigned int GetAlign() const override;
    int GetResourceSlot() const { return resource_.slot; }
private:
    fates::decomp_detail::EffectResourceHandle resource_{};
};
