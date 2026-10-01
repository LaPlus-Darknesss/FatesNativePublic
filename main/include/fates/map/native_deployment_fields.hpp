#pragma once
#include <array>
#include <cstdint>
namespace fates::map::native {
using DeploymentBits=std::array<std::uint8_t,128>;
using DeploymentMovementPlane=std::array<std::int8_t,1024>;
struct DeploymentRangePlanes {
    DeploymentBits attack{},rod{};
    bool operator==(const DeploymentRangePlanes&) const=default;
};
struct DeploymentFields {DeploymentMovementPlane movement{};DeploymentRangePlanes ranges{};};
}
