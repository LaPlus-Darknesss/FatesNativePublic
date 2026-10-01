#pragma once
#include <cstdint>
#include "fates/graphics/basic_types.hpp"

namespace nn::gr::CTR { class CommandBufferJumpHelper; }
class ModelState;
class ModelCallback { public: struct ModelArgs {}; };

namespace fates::decomp_detail {
struct ModelStateData {
    ModelCallback* callback{};
    std::uint16_t flags{};
    std::uint16_t stageMask{};
    int constantColorIndex{};
    Color8 additiveColor{};
    std::uint32_t extra[3]{};
};
void InitializeModelStateDefaults(ModelStateData& state);
void EmitModelStateBegin(const ModelStateData& state,nn::gr::CTR::CommandBufferJumpHelper& helper,const ModelCallback::ModelArgs& args);
void EmitModelStateEnd(const ModelStateData& state,nn::gr::CTR::CommandBufferJumpHelper& helper,const ModelCallback::ModelArgs& args);
}
