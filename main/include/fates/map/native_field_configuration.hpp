#pragma once
#include <array>
#include <cstdint>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <string_view>

namespace fates::map::native {
// E2: original 196-byte FieldConfig, corroborated by Paragon Field.yml.
// Strings are opaque CP932 bytes; null and present-empty remain distinct.
// Unconsumed numeric fields are retained without inventing their behavior.
struct FieldConfiguration {
    std::optional<std::string> field_name,environment_name,sky_name;
    std::optional<std::string> switch_field,block_object,environment_sound;
    std::array<std::uint8_t,172> numeric{}; // original offsets 0x18..0xC3
    std::uint8_t Weather() const noexcept {return numeric[7];}
    std::uint8_t Reverb() const noexcept {return numeric[0];}
    std::uint8_t ShadowMap() const noexcept {return numeric[3];}
    float TimeZone() const noexcept;
};
// Header+8 supplies the config pointer. A null config is a valid archive value;
// missing/malformed headers refuse atomically rather than following unsafe ARM.
bool DecodeFieldConfiguration(std::span<const std::uint8_t>,std::shared_ptr<const FieldConfiguration>&);
std::string FieldConfigurationPath(std::string_view);
struct FieldConfigurationResource {virtual ~FieldConfigurationResource()=default;};
using FieldConfigurationResourceRef=std::shared_ptr<FieldConfigurationResource>;
struct FieldConfigurationRead {
    FieldConfigurationResourceRef resource;
    bool ready{};
    std::shared_ptr<const FieldConfiguration> configuration;
};
class FieldConfigurationBackend {
public:
    virtual ~FieldConfigurationBackend()=default;
    virtual bool ConfigurationExists(std::string_view path)=0;
    // Established FileBase Open/Entry/Close TryImmediate (2) transport boundary.
    // Ready reads have a valid decoded header; a null header pointer is unsafe.
    virtual FieldConfigurationRead ReadConfiguration(std::string_view path,const FieldConfigurationResourceRef& current)=0;
    virtual void ReleaseConfiguration(FieldConfigurationResourceRef&)=0;
};
// FieldData::LoadParam returns requested-path existence, NOT read readiness.
// A failed read leaves the prior config pointer unchanged. FreeParam releases
// its resource before clearing the config. Services are synchronous/nonthrowing.
class FieldConfigurationOwner {
public:
    explicit FieldConfigurationOwner(FieldConfigurationBackend& backend):backend_(backend){}
    ~FieldConfigurationOwner(){Free();}
    FieldConfigurationOwner(const FieldConfigurationOwner&)=delete;
    FieldConfigurationOwner& operator=(const FieldConfigurationOwner&)=delete;
    bool Load(std::string_view);
    void Free();
    const auto& Configuration() const noexcept {return configuration_;}
private:
    FieldConfigurationBackend& backend_;
    FieldConfigurationResourceRef resource_;
    std::shared_ptr<const FieldConfiguration> configuration_;
};
}
