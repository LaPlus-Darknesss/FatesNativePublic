#pragma once

class FileObject;
class UniqueArchiveObject;

namespace fates::decomp_detail {

// TexFile's former opaque resource helpers were removed in Pass 17 after
// ResObject/TexObject/ITexture became durable source. StructFile's owning
// resource subtype remains intentionally deferred.
const UniqueArchiveObject* GetStructArchiveObject(const FileObject& object);

} // namespace fates::decomp_detail
