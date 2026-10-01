#pragma once

#include "fates/detail/arm32_address.hpp"

#include <cstddef>
#include <cstdint>

class Tutorial {
public:
    fates::decomp_detail::Arm32Address unknown00{};       // +0x00
    std::uint16_t unknown04{};                            // +0x04
    std::int16_t variantMode{};                           // +0x06
    fates::decomp_detail::Arm32Address nameMessage{};     // +0x08, GetName
    fates::decomp_detail::Arm32Address messageIdentifier{}; // +0x0C, GetHelp/GetMessage
    fates::decomp_detail::Arm32Address resourceName{};    // +0x10, GetFileName

    static void Initialize(const void* data, int count);
    static Tutorial* Get(const char* identifier);
    static Tutorial* Get(int index);
    static int GetNum();
    static void Finalize();

    const wchar_t* GetMessage(int page) const;
    int GetPageNum() const;
    void GetFileName(char* destination, int capacity) const;
    const wchar_t* GetHelp() const;
    const wchar_t* GetName() const;
};

static_assert(sizeof(Tutorial) == 0x14);
static_assert(offsetof(Tutorial, variantMode) == 0x06);
static_assert(offsetof(Tutorial, nameMessage) == 0x08);
static_assert(offsetof(Tutorial, messageIdentifier) == 0x0C);
static_assert(offsetof(Tutorial, resourceName) == 0x10);
