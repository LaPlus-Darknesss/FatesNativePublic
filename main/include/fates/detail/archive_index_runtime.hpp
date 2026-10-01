#pragma once

class IdentHash;

namespace fates::decomp_detail {

// Retail passes null in several callsites to request the process-global
// archive identifier table. Its higher-level owner has not yet been promoted.
IdentHash* GetGlobalArchiveIdentHash();

} // namespace fates::decomp_detail
