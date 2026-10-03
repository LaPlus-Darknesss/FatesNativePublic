#pragma once
#include "fates/io/native_texture_objects.hpp"
namespace fates::graphics::portable {
// Reuses the same BCH and packed-pixel decoders as the field assets. No GPU,
// game cache, process state, or asynchronous completion is owned here.
io::native::NativeTextureResourceBackend CreatePortableTextureResourceBackend();
}
