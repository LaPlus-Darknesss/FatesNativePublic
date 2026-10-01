#pragma once

#include "fates/detail/render_state_runtime.hpp"
#include "fates/graphics/command_alter.hpp"

class SceneSystem;

class RenderState {
public:
    class Callback : public CommandAlter::Callback {
    public:
        explicit Callback(RenderState& owner) : owner_(&owner) {}
        void OnReplace() override;
    private:
        RenderState* owner_{};
    };

    explicit RenderState(SceneSystem* scene);
    ~RenderState();

    void BeginCommand();
    void EndCommand();
    void Change(GfxConst::Type type);
    void BeginDraw(bool persistent);
    void EndDraw(bool persistent);

private:
    friend class Callback;
    fates::decomp_detail::RenderStateRuntime runtimeState_{};
};
