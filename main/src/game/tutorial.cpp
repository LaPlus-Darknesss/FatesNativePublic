#include "fates/game/tutorial.hpp"

#include "fates/detail/file_runtime.hpp"
#include "fates/detail/message_runtime.hpp"
#include "fates/detail/metadata_runtime.hpp"
#include "fates/engine/ident_hash.hpp"
#include "fates/game/message.hpp"

#include <cstdio>

namespace {
Tutorial* gTutorials = nullptr;
int gTutorialCount = 0;

const char* VariantSuffix(const Tutorial& tutorial) {
    switch (tutorial.variantMode) {
    case 1:
        return fates::decomp_detail::IsTutorialCasualMode()
            ? "_Casual"
            : "";
    case 2:
        return fates::decomp_detail::IsTutorialSimpleMode()
            ? "_Simple"
            : "";
    case 3:
        return fates::decomp_detail::IsTutorialEntrustMode()
            ? "_Entrust"
            : "";
    default:
        return "";
    }
}

const char* TargetString(fates::decomp_detail::Arm32Address address) {
    return fates::decomp_detail::TargetPointer<const char>(address);
}
} // namespace

void Tutorial::Initialize(const void* data, int count) {
    gTutorials = const_cast<Tutorial*>(
        static_cast<const Tutorial*>(data));
    gTutorialCount = count;
}

Tutorial* Tutorial::Get(const char* identifier) {
    if (fates::decomp_detail::gTutorialIdentHash == nullptr) {
        return nullptr;
    }
    return static_cast<Tutorial*>(
        fates::decomp_detail::gTutorialIdentHash->GetSurely(identifier));
}

Tutorial* Tutorial::Get(int index) {
    return gTutorials + index;
}

int Tutorial::GetNum() {
    return gTutorialCount;
}

void Tutorial::Finalize() {
    if (gTutorials != nullptr) {
        gTutorialCount = 0;
        gTutorials = nullptr;
    }
}

const wchar_t* Tutorial::GetMessage(int page) const {
    const char* const base = TargetString(messageIdentifier);
    if (base == nullptr) {
        return nullptr;
    }

    char identifier[0x40]{};
    std::snprintf(
        identifier,
        sizeof(identifier),
        "%s%s_%02d",
        base,
        VariantSuffix(*this),
        page);

    if (Mess::IsExist(identifier)) {
        return Mess::Get(identifier);
    }

    std::snprintf(
        identifier,
        sizeof(identifier),
        "%s_%02d",
        base,
        page);
    return Mess::Get(identifier);
}

int Tutorial::GetPageNum() const {
    const char* const base = TargetString(messageIdentifier);
    if (base == nullptr) {
        return 0;
    }

    int page = 0;
    char identifier[0x40]{};
    for (;; ++page) {
        std::snprintf(
            identifier,
            sizeof(identifier),
            "%s_%02d",
            base,
            page);
        if (!Mess::IsExist(identifier)) {
            return page;
        }
    }
}

void Tutorial::GetFileName(char* destination, int capacity) const {
    const char* const resource = TargetString(resourceName);
    if (resource != nullptr) {
        std::snprintf(
            destination,
            static_cast<std::size_t>(capacity),
            "tut/%s%s.bch.lz",
            resource,
            VariantSuffix(*this));
        if (fates::decomp_detail::FileExists(destination)) {
            return;
        }

        std::snprintf(
            destination,
            static_cast<std::size_t>(capacity),
            "tut/%s.bch.lz",
            resource);
        if (fates::decomp_detail::FileExists(destination)) {
            return;
        }
    }

    std::snprintf(
        destination,
        static_cast<std::size_t>(capacity),
        "tut/000_SystemMenu.bch.lz");
}

const wchar_t* Tutorial::GetHelp() const {
    const char* const identifier = TargetString(messageIdentifier);
    return identifier != nullptr ? Mess::Get(identifier) : nullptr;
}

const wchar_t* Tutorial::GetName() const {
    return Mess::Get(TargetString(nameMessage));
}
