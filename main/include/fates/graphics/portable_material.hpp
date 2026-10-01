#pragma once
#include "fates/graphics/portable_bch.hpp"

namespace fates::graphics::portable {
struct MaterialRegister {
    std::uint32_t value{},known_bits{};
    // A consumer asks only for the bits its field actually uses. Missing initial
    // register state never becomes an invented zero/default.
    std::optional<std::uint32_t> Read(std::uint32_t needed_bits) const noexcept {
        if((known_bits&needed_bits)!=needed_bits)return {};return value&needed_bits;
    }
};
using MaterialRegisters=std::map<unsigned,MaterialRegister>;
MaterialRegisters ComposeMaterialRegisters(std::span<const BchCommandWrite>);
struct BchMaterial {
    unsigned model_index{},material_index{};
    std::optional<std::string> name;
    std::array<std::optional<std::string>,3> textures;
    std::size_t parameters_offset{};
    std::optional<std::size_t> mapper_offset;
    std::vector<BchCommandWrite> texture_commands,fragment_commands;
};
// Lab6 exact wrapper/name relationships and Lab7 texture/fragment command
// ownership. No preview-albedo heuristic or guessed texture name fallback.
bool ReadBchMaterials(const BchFile&,std::vector<BchMaterial>&,std::string& error);
}
