#pragma once
#include "fates/detail/model_state_runtime.hpp"

class ModelState {
public:
    ModelState();
    void UpdateState();
    void BeginCommand(nn::gr::CTR::CommandBufferJumpHelper&, const ModelCallback::ModelArgs&) const;
    void EndCommand(nn::gr::CTR::CommandBufferJumpHelper&, const ModelCallback::ModelArgs&) const;
protected:
    fates::decomp_detail::ModelStateData modelState_{};
};
