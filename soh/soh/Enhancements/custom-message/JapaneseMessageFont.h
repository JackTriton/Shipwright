#pragma once

#include <cstdint>

// Character mapping only. The native Font loader owns the texture resources.
namespace JapaneseMessageFont {
// Returns zero for an unavailable glyph. Printable ASCII uses the matching
// Japanese full-width glyph; an ordinary space stays on the native space path.
uint16_t ToShiftJIS(uint32_t scalar);
} // namespace JapaneseMessageFont
