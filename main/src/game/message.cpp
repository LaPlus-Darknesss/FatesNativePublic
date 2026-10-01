#include "fates/game/message.hpp"

#include "fates/detail/file_runtime.hpp"
#include "fates/detail/message_runtime.hpp"
#include "fates/detail/metadata_runtime.hpp"
#include "fates/engine/ident_hash.hpp"

#include <array>
#include <cstddef>
#include <cstdio>
#include <cwchar>

namespace {

constexpr int kMessagePathCapacity = 0x50;
constexpr int kDefaultMessageBufferSize = 0x400;
constexpr int kArgumentCount = 4;
constexpr int kArgumentBufferSize = 0x40;
constexpr int kLinkNameCount = 2;
constexpr int kLinkNameSize = 0x0D;

wchar_t gDefaultMessageBuffer[kDefaultMessageBufferSize]{};
wchar_t* gMessageBuffer = gDefaultMessageBuffer;
int gMessageBufferSize = kDefaultMessageBufferSize;

wchar_t gArgumentBuffers[kArgumentCount][kArgumentBufferSize]{};
int gArgumentBufferSizes[kArgumentCount] = {
    kArgumentBufferSize,
    kArgumentBufferSize,
    kArgumentBufferSize,
    kArgumentBufferSize,
};

wchar_t gLinkNames[kLinkNameCount][kLinkNameSize]{};
char gMessagePath[kMessagePathCapacity]{};
constexpr wchar_t kEmptyMessage[] = L"";

int NormalizeArgumentIndex(int index) {
    return index >= 0 && index < kArgumentCount ? index : 0;
}

void CopyWide(wchar_t* destination, const wchar_t* source, int capacity) {
    if (destination == nullptr || capacity <= 0) {
        return;
    }
    if (source == nullptr) {
        destination[0] = L'\0';
        return;
    }
    std::wcsncpy(destination, source, static_cast<std::size_t>(capacity));
    destination[capacity - 1] = L'\0';
}

void AppendWide(wchar_t* destination, const wchar_t* source, int capacity) {
    if (destination == nullptr || source == nullptr || capacity <= 0) {
        return;
    }
    const std::size_t used = std::wcslen(destination);
    if (used >= static_cast<std::size_t>(capacity - 1)) {
        return;
    }
    std::wcsncat(
        destination,
        source,
        static_cast<std::size_t>(capacity - 1) - used);
}

const wchar_t* LookupMessage(const char* identifier) {
    if (identifier == nullptr || fates::decomp_detail::gMessageIdentHash == nullptr) {
        return nullptr;
    }
    return static_cast<const wchar_t*>(
        fates::decomp_detail::gMessageIdentHash->GetSurely(identifier));
}

const char* CreateMessageFileName(
    const char* routeDirectory,
    const char* archiveName) {
    if (routeDirectory == nullptr) {
        std::snprintf(
            gMessagePath,
            sizeof(gMessagePath),
            "m/%s.bin.lz",
            archiveName);
    } else {
        std::snprintf(
            gMessagePath,
            sizeof(gMessagePath),
            "m/%s/%s.bin.lz",
            routeDirectory,
            archiveName);
    }

    fates::decomp_detail::ApplyLanguageFolder(gMessagePath);
    return gMessagePath;
}

bool CopyExpansion(
    wchar_t*& output,
    wchar_t* outputEnd,
    const wchar_t* text) {
    if (text == nullptr) {
        return true;
    }
    const std::size_t length = std::wcslen(text);
    if (output + length >= outputEnd) {
        *output = L'\0';
        return false;
    }
    std::wmemcpy(output, text, length);
    output += length;
    return true;
}

void MakeArgedMessage(
    const char* identifier,
    const wchar_t* input,
    wchar_t* output) {
    (void)identifier;
    if (input == nullptr || output == nullptr) {
        return;
    }

    wchar_t* const outputEnd = output + gMessageBufferSize - 1;

    while (*input != L'\0' && output < outputEnd) {
        if (input[0] == L'$' && input[1] == L'a') {
            int index = static_cast<int>(input[2] - L'0');
            index = NormalizeArgumentIndex(index);
            if (!CopyExpansion(output, outputEnd, gArgumentBuffers[index])) {
                return;
            }
            input += 3;
            continue;
        }

        if (input[0] == L'$' && input[1] == L'N') {
            if (input[2] == L'l') {
                int index = static_cast<int>(input[3] - L'0');
                if (index < 0 || index >= kLinkNameCount) {
                    index = 0;
                }
                if (!CopyExpansion(output, outputEnd, gLinkNames[index])) {
                    return;
                }
                input += 4;
                continue;
            }

            if (input[2] == L'u') {
                const wchar_t* name =
                    fates::decomp_detail::GetPlayerUnitName();
                if (name == nullptr) {
                    // Literal retail identifier: MPID_デフォルト名
                    // (MPID_DefaultName / default player name).
                    name = Mess::Get("MPID_デフォルト名");
                }
                if (!CopyExpansion(output, outputEnd, name)) {
                    return;
                }
                input += 3;
                continue;
            }
        }

        *output++ = *input++;
    }

    if (output >= outputEnd && *input != L'\0') {
        *output = L'\0';
        return;
    }
    *output = L'\0';
}

void SetArchivePointer(const char* name, const char* path) {
    if (path == nullptr || !fates::decomp_detail::FileExists(path)) {
        return;
    }
    void* const archive =
        fates::decomp_detail::LoadMessageArchiveFile(path);
    fates::decomp_detail::SetMessageArchivePointer(name, archive);
}

void FreeArchivePath(const char* path) {
    if (path != nullptr && fates::decomp_detail::FileExists(path)) {
        fates::decomp_detail::FreeMessageArchiveFile(path);
    }
}

} // namespace

void Mess::Initialize() {
    fates::decomp_detail::InitializeMessageArchiveRegistry();
}

bool Mess::IsFileLoad(const char* archiveName) {
    return fates::decomp_detail::IsMessageArchiveBound(archiveName);
}

void Mess::SetArgument(int index, const wchar_t* text) {
    index = NormalizeArgumentIndex(index);
    CopyWide(
        gArgumentBuffers[index],
        text,
        gArgumentBufferSizes[index]);
}

void Mess::SetArgument(int index, int value) {
    wchar_t buffer[0x10]{};
    std::swprintf(buffer, 0x10, L"%d", value);
    SetArgument(index, buffer);
}

void Mess::SetLinkName(int index, const wchar_t* text) {
    // Retail has no bounds check here; valid callers use slots 0 and 1.
    CopyWide(gLinkNames[index], text, kLinkNameSize);
}

const wchar_t* Mess::GetNoReplace(const char* identifier) {
    const wchar_t* message = LookupMessage(identifier);
    if (message == nullptr) {
        return kEmptyMessage;
    }

    char commonIdentifier[0x50]{};
    std::snprintf(
        commonIdentifier,
        sizeof(commonIdentifier),
        "%s_COM",
        identifier);
    const wchar_t* const common = LookupMessage(commonIdentifier);

    if (common != nullptr) {
        CopyWide(gMessageBuffer, common, gMessageBufferSize);
    }

    if (message[0] == L'$' && message[1] == L'a') {
        return message + 2;
    }

    if (common == nullptr) {
        return message;
    }

    AppendWide(gMessageBuffer, message, gMessageBufferSize);
    return gMessageBuffer;
}

void Mess::FileFreeRoute(const char* archiveName) {
    const char* const routeDirectory =
        fates::decomp_detail::GetCurrentRouteDirectoryName();

    if (!fates::decomp_detail::UnbindMessageArchive(archiveName)) {
        return;
    }

    const char* path = nullptr;
    if (routeDirectory != nullptr) {
        path = CreateMessageFileName(routeDirectory, archiveName);
        if (!fates::decomp_detail::FileExists(path)) {
            path = nullptr;
        }
    }
    if (path == nullptr) {
        path = CreateMessageFileName(nullptr, archiveName);
        if (!fates::decomp_detail::FileExists(path)) {
            return;
        }
    }

    fates::decomp_detail::FreeMessageArchiveFile(path);

    if (routeDirectory == nullptr) {
        std::snprintf(
            gMessagePath,
            sizeof(gMessagePath),
            "m/common/%s.bin.lz",
            archiveName);
        FreeArchivePath(gMessagePath);
    }
}

void Mess::FileLoadRoute(const char* archiveName) {
    const char* const routeDirectory =
        fates::decomp_detail::GetCurrentRouteDirectoryName();

    if (!fates::decomp_detail::BindMessageArchive(archiveName)) {
        return;
    }

    const char* path = nullptr;
    if (routeDirectory != nullptr) {
        path = CreateMessageFileName(routeDirectory, archiveName);
        if (!fates::decomp_detail::FileExists(path)) {
            path = nullptr;
        }
    }
    if (path == nullptr) {
        path = CreateMessageFileName(nullptr, archiveName);
        if (!fates::decomp_detail::FileExists(path)) {
            return;
        }
    }

    SetArchivePointer(archiveName, path);

    if (routeDirectory == nullptr) {
        std::snprintf(
            gMessagePath,
            sizeof(gMessagePath),
            "m/common/%s.bin.lz",
            archiveName);
        SetArchivePointer(archiveName, gMessagePath);
    }
}

int Mess::GetBufferSize() {
    return gMessageBufferSize;
}

bool Mess::IsFileExistRoute(const char* archiveName) {
    const char* const routeDirectory =
        fates::decomp_detail::GetCurrentRouteDirectoryName();

    if (routeDirectory != nullptr &&
        fates::decomp_detail::FileExists(
            CreateMessageFileName(routeDirectory, archiveName))) {
        return true;
    }

    return fates::decomp_detail::FileExists(
        CreateMessageFileName(nullptr, archiveName));
}

wchar_t* Mess::GetArgumentBuffer(int index) {
    index = NormalizeArgumentIndex(index);
    return gArgumentBuffers[index];
}

int Mess::GetArgumentBufferSize(int index) {
    // Retail does not clamp this accessor; callers are expected to pass 0..3.
    return gArgumentBufferSizes[index];
}

const wchar_t* Mess::Get(const char* identifier) {
    const wchar_t* message = LookupMessage(identifier);
    if (message == nullptr) {
        return kEmptyMessage;
    }

    char commonIdentifier[0x50]{};
    std::snprintf(
        commonIdentifier,
        sizeof(commonIdentifier),
        "%s_COM",
        identifier);
    const wchar_t* const common = LookupMessage(commonIdentifier);

    int commonLength = 0;
    if (common != nullptr) {
        commonLength = static_cast<int>(std::wcslen(common));
        CopyWide(gMessageBuffer, common, gMessageBufferSize);
    }

    if (message[0] == L'$' && message[1] == L'a') {
        MakeArgedMessage(
            identifier,
            message + 2,
            gMessageBuffer + commonLength);
        return gMessageBuffer;
    }

    if (common == nullptr) {
        return message;
    }

    AppendWide(gMessageBuffer, message, gMessageBufferSize);
    return gMessageBuffer;
}

bool Mess::IsExist(const char* identifier) {
    return LookupMessage(identifier) != nullptr;
}

void Mess::FileFree(const char* archiveName) {
    if (!fates::decomp_detail::UnbindMessageArchive(archiveName)) {
        return;
    }

    const char* const path =
        CreateMessageFileName(nullptr, archiveName);
    if (!fates::decomp_detail::FileExists(path)) {
        return;
    }

    fates::decomp_detail::FreeMessageArchiveFile(path);

    std::snprintf(
        gMessagePath,
        sizeof(gMessagePath),
        "m/common/%s.bin.lz",
        archiveName);
    FreeArchivePath(gMessagePath);
}

void Mess::FileLoad(const char* archiveName) {
    if (!fates::decomp_detail::BindMessageArchive(archiveName)) {
        return;
    }

    const char* const path =
        CreateMessageFileName(nullptr, archiveName);
    if (!fates::decomp_detail::FileExists(path)) {
        return;
    }

    SetArchivePointer(archiveName, path);

    std::snprintf(
        gMessagePath,
        sizeof(gMessagePath),
        "m/common/%s.bin.lz",
        archiveName);
    SetArchivePointer(archiveName, gMessagePath);
}

void Mess::Finalize() {
    fates::decomp_detail::FinalizeMessageArchiveRegistry();
}

wchar_t* Mess::GetBuffer() {
    return gMessageBuffer;
}

void Mess::SetBuffer(wchar_t* buffer, int size) {
    if (buffer == nullptr) {
        gMessageBuffer = gDefaultMessageBuffer;
        gMessageBufferSize = kDefaultMessageBufferSize;
        return;
    }

    gMessageBuffer = buffer;
    gMessageBufferSize = size;
}
