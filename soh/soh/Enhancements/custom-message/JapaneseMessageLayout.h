#pragma once

#include <cstdint>
#include <string_view>

// OoTR TextBox.py uses 16/14 full-width cells (normal/icon) and three lines.
// Work in screen pixels at the native Japanese 88% scale, using the same
// integer pen movement as Message_DrawTextJPN. Rendering shares these metrics.
namespace JapaneseMessageLayout {
constexpr int LinesPerPage = 3;
constexpr float FontScale = 0.88f;
constexpr int LineWidth(bool icon) {
    return (icon ? 16 * 14 : 16 * 16) * 88 / 100;
}

inline constexpr float AsciiWidths[] = {
#include "NativeMessageAsciiWidths.inc"
};
static_assert(sizeof(AsciiWidths) / sizeof(AsciiWidths[0]) == 95);

constexpr bool IsAscii(uint32_t scalar) {
    return scalar >= 0x21 && scalar <= 0x7E;
}

constexpr bool IsAsciiWord(uint32_t scalar) {
    return (scalar >= 'A' && scalar <= 'Z') || (scalar >= 'a' && scalar <= 'z') || (scalar >= '0' && scalar <= '9') ||
           scalar == '\'' || scalar == '"';
}

constexpr uint32_t Canonical(uint32_t scalar) {
    if (scalar == U'＂')
        return U'”';
    if (scalar == U'＇')
        return U'’';
    return scalar;
}

constexpr int DrawOffset(uint32_t scalar) {
    switch (Canonical(scalar)) {
        case U'（':
        case U'「':
            return -6;
        case U'・':
            return -3;
        case U'？':
        case U'！':
        case U'＾':
        case U'１':
            return -2;
        default:
            return 0;
    }
}

constexpr int Advance(uint32_t scalar, float scale = FontScale) {
    // Ordinary ASCII uses the native Latin font, not a full-width alias.
    // Keep the integer rounding identical to the game's draw loop.
    if (IsAscii(scalar))
        return static_cast<int>(AsciiWidths[scalar - ' '] * scale);
    int advance;
    switch (Canonical(scalar)) {
        case U'　':
        case ' ':
            return 6;
        case U'．':
            advance = 3;
            break;
        case U'）':
        case U'」':
            advance = 5;
            break;
        case U'、':
        case U'。':
        case U'”':
            advance = 7;
            break;
        case U'＾':
        case U'＃':
        case U'＊':
            advance = 9;
            break;
        case U'・':
            advance = 10;
            break;
        default:
            advance = static_cast<int>(16 * scale);
            break;
    }
    return DrawOffset(scalar) + advance;
}

inline bool CannotStartLine(uint32_t scalar) {
    constexpr std::u32string_view punctuation =
        U",.!?:;)]}、。，．？！：；）］｝」』】〉》〕〗〙〛’”ー々ヽヾゝゞァィゥェォッャュョヮぁぃぅぇぉっゃゅょゎ";
    return punctuation.find(static_cast<char32_t>(Canonical(scalar))) != std::u32string_view::npos;
}

inline bool CannotEndLine(uint32_t scalar) {
    constexpr std::u32string_view punctuation = U"([{（［｛「『【〈《〔〖〘〚‘“";
    return punctuation.find(static_cast<char32_t>(Canonical(scalar))) != std::u32string_view::npos;
}
} // namespace JapaneseMessageLayout
