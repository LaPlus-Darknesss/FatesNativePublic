#pragma once

class IdentHash;

namespace fates::decomp_detail {

// These are source-facing names for the retail identifier hashes used by
// the compact GameData families below. Their exact target storage addresses
// stay in Pass 7 provenance until the owning manager/global-storage layer is
// reconstructed.
extern IdentHash* gEquipSkillIdentHash;
extern IdentHash* gAffiliationIdentHash; // retail Belong
extern IdentHash* gChapterIdentHash;

} // namespace fates::decomp_detail
