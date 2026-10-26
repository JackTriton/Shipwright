#include "GameMessageGlyphs.h"
#include "UnicodeMessage.h"
#include "JapaneseMessageFont.h"
#include "JapaneseMessageLayout.h"
#include "soh/Enhancements/Lang/MessageLanguage.h"
#include "soh/Enhancements/Lang/UIFontGlyphs.h"
#include "soh/OTRGlobals.h"
#include <array>
#include <cstring>

extern "C" {
#include "functions.h"
#include "textures/nes_font_static/nes_font_static.h"
}

namespace {
std::array<float, 200> advances{};
std::array<uint32_t, 200> japaneseScalars{};
}
void GameMessage_BeginPage(void) {
    advances.fill(0);
    japaneseScalars.fill(0);
}

size_t GameMessage_DecodeGlyph(const char* input, size_t length, uint8_t* texture, uint16_t decodedIndex) {
    if (!length || decodedIndex >= advances.size())
        return 0;
    const unsigned char first = input[0];
    if (first >= 0x80 && first <= 0xAB)
        return 0; // Existing native button/Latin glyph.
    uint32_t scalar;
    const auto bytes = UnicodeMessage::ReadScalar(std::string_view(input, length), 0, scalar);
    std::memset(texture, 0, 128);
    advances[decodedIndex] = 16;
    if (MessageLanguage_Get() == LANGUAGE_JPN) {
        japaneseScalars[decodedIndex] = scalar;
        const uint16_t character = JapaneseMessageFont::ToShiftJIS(scalar);
        // Reuse the same resource reference as Font_LoadCharWide, including
        // native texture replacements. Never rasterize a UI font for Japanese.
        const char* path = Font_GetCharWideTexture(character ? character : 0x81AC); // geta mark
        if (path == nullptr)
            path = gMsgChar3FQuestionMarkTex; // Japanese assets unavailable (PAL-only).
        std::memcpy(texture, path, std::strlen(path) + 1);
        return bytes;
    }
    if (OTRGlobals::Instance != nullptr && OTRGlobals::Instance->fontJapanese != nullptr) {
        static Lang::UIFontGlyphs glyphs(OTRGlobals::Instance->fontJapanese);
        const auto& cell = glyphs.GetCell(scalar);
        std::memcpy(texture, cell.pixels.data(), cell.pixels.size());
        advances[decodedIndex] = cell.advance;
    }
    return bytes;
}
float GameMessage_GetAdvance(uint16_t decodedIndex, float legacyAdvance) {
    return decodedIndex < advances.size() && advances[decodedIndex] > 0 ? advances[decodedIndex] : legacyAdvance;
}
int16_t GameMessage_GetDrawOffset(uint16_t decodedIndex) {
    return decodedIndex < japaneseScalars.size() && japaneseScalars[decodedIndex]
               ? JapaneseMessageLayout::DrawOffset(japaneseScalars[decodedIndex])
               : 0;
}
float GameMessage_GetScaledAdvance(uint16_t decodedIndex, float legacyAdvance, float scale) {
    return decodedIndex < japaneseScalars.size() && japaneseScalars[decodedIndex]
               ? JapaneseMessageLayout::Advance(japaneseScalars[decodedIndex], scale)
               : GameMessage_GetAdvance(decodedIndex, legacyAdvance) * scale;
}
