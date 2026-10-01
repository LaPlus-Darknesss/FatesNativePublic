#pragma once

#include "fates/detail/archive_table_runtime.hpp"

namespace fates::decomp_detail {

// Compatibility name retained for the earlier Person lookup reconstruction.
// Pass 5 establishes that the same 3-word node pattern is shared by Person,
// retail Job (class data), and Item.
using LoadedPersonArchive = LoadedTableArchive;

} // namespace fates::decomp_detail
