#include "fates/graphics/render_state.hpp"

RenderState::RenderState(SceneSystem* scene) {
    fates::decomp_detail::InitializeRenderState(runtimeState_, scene);
}

RenderState::~RenderState() {
    fates::decomp_detail::FinalizeRenderState(runtimeState_);
}

void RenderState::BeginCommand() {
    Change(static_cast<GfxConst::Type>(0));
}

void RenderState::EndCommand() {
    // Retail finalizes any active CommandAlter jump segment, conditionally
    // emits the effect-camera replacement block, clears cached material/light/
    // fog state, then returns to command type zero.
    fates::decomp_detail::EndRenderCommand(runtimeState_);
}

void RenderState::Change(GfxConst::Type type) {
    // This is first-party state orchestration around Nintendo command-buffer
    // helpers.  The adapter owns raw jump-pointer/PICA commands; the source API
    // keeps the state transition itself explicit.
    fates::decomp_detail::ChangeRenderState(runtimeState_, type);
}

void RenderState::BeginDraw(bool persistent) {
    fates::decomp_detail::BeginRenderDraw(runtimeState_, persistent);
}

void RenderState::EndDraw(bool persistent) {
    fates::decomp_detail::EndRenderDraw(runtimeState_, persistent);
}

void RenderState::Callback::OnReplace() {
    if (owner_ != nullptr) {
        fates::decomp_detail::ReplayRenderStateReplacement(owner_->runtimeState_);
    }
}
