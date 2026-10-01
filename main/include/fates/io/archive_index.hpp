#pragma once

class IdentHash;

// Retail free functions used by archive-backed data owners. ArchiveConstruct
// relocates pointer fields in-place (once) and registers identifier records.
// ArchiveDestruct unregisters those identifiers but intentionally does not
// reverse the pointer relocation.
void* ArchiveConstruct(void* archive, IdentHash* hash = nullptr);
void ArchiveDestruct(void* archive, IdentHash* hash = nullptr);
