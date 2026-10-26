#include "UIFontGlyphs.h"
#include <imgui_internal.h>
#include <algorithm>
#include <cmath>

namespace Lang {
bool RequiresUnicodeUIFont(std::string_view text) {
    if (text.empty())
        return false;
    const char* cursor = text.data();
    const char* end = cursor + text.size();
    while (cursor < end) {
        unsigned int codepoint;
        const int bytes = ImTextCharFromUtf8(&codepoint, cursor, end);
        if (bytes <= 0 || codepoint == 0) {
            break;
        }
        if (codepoint > 0xFF) {
            return true;
        }
        cursor += bytes;
    }
    return false;
}

UIFontGlyphs::UIFontGlyphs(ImFont* font) : font(font) {
}

float UIFontGlyphs::FontSize() const {
    return font->FontSize;
}

const UIFontGlyphs::Glyph& UIFontGlyphs::Get(unsigned int codepoint) {
    if (const auto found = glyphs.find(codepoint); found != glyphs.end()) {
        return found->second;
    }
    Glyph result;
    const auto character = codepoint <= IM_UNICODE_CODEPOINT_MAX ? codepoint : IM_UNICODE_CODEPOINT_INVALID;
    const auto* glyph = font->FindGlyph(static_cast<ImWchar>(character));
    if (glyph != nullptr) {
        result.x = glyph->X0;
        result.y = glyph->Y0;
        result.advance = glyph->AdvanceX;
        if (glyph->Visible) {
            unsigned char* atlas;
            int atlasWidth, atlasHeight;
            font->ContainerAtlas->GetTexDataAsAlpha8(&atlas, &atlasWidth, &atlasHeight);
            const int left = std::lround(glyph->U0 * atlasWidth);
            const int top = std::lround(glyph->V0 * atlasHeight);
            const int sourceWidth = std::lround(glyph->U1 * atlasWidth) - left;
            const int sourceHeight = std::lround(glyph->V1 * atlasHeight) - top;
            const int width = std::ceil(glyph->X1 - glyph->X0);
            result.height = std::ceil(glyph->Y1 - glyph->Y0);
            result.width = (width + 7) & ~7;
            result.pixels.resize(result.width * result.height, 0);
            // Atlas UVs include ImGui's horizontal/vertical oversampling. They
            // are not logical glyph dimensions: treating them as such stretches
            // the text and makes neighboring glyphs overlap (OversampleH is 3).
            for (int row = 0; row < result.height; ++row)
                for (int column = 0; column < width; ++column) {
                    const int x0 = column * sourceWidth / width;
                    const int x1 = std::max(x0 + 1, (column + 1) * sourceWidth / width);
                    const int y0 = row * sourceHeight / result.height;
                    const int y1 = std::max(y0 + 1, (row + 1) * sourceHeight / result.height);
                    unsigned int sum = 0;
                    for (int y = y0; y < y1; ++y)
                        for (int x = x0; x < x1; ++x)
                            sum += atlas[(top + y) * atlasWidth + left + x];
                    result.pixels[row * result.width + column] = sum / ((x1 - x0) * (y1 - y0));
                }
        }
    }
    return glyphs.emplace(codepoint, std::move(result)).first->second;
}

float UIFontGlyphs::CellEmSize() const {
    const auto* fullWidthSpace = font->FindGlyphNoFallback(0x3000);
    return fullWidthSpace ? fullWidthSpace->AdvanceX : font->FontSize;
}

const UIFontGlyphs::Cell& UIFontGlyphs::GetCell(unsigned int codepoint) {
    if (const auto found = cells.find(codepoint); found != cells.end())
        return found->second;
    const auto& glyph = Get(codepoint);
    Cell result;
    const float em = CellEmSize();
    const float scale = 16.0f / em;
    const auto* reference = font->FindGlyphNoFallback(0x56FD); // 国: reference CJK em box.
    const float topOffset = reference ? reference->Y0 - (em - (reference->Y1 - reference->Y0)) / 2 : 0;
    result.advance = glyph.advance * scale;
    // Bilinear sampling preserves the font's aspect ratio and baseline while
    // quantizing alpha to the exact I4 format used by the original renderer.
    for (int y = 0; y < 16; ++y) {
        for (int x = 0; x < 16; ++x) {
            const float sx = (x + 0.5f) / scale - glyph.x - 0.5f;
            const float sy = (y + 0.5f) / scale - (glyph.y - topOffset) - 0.5f;
            const int left = std::floor(sx), top = std::floor(sy);
            float alpha = 0;
            for (int dy = 0; dy < 2; ++dy)
                for (int dx = 0; dx < 2; ++dx) {
                    const int px = left + dx, py = top + dy;
                    if (px >= 0 && px < glyph.width && py >= 0 && py < glyph.height)
                        alpha += glyph.pixels[py * glyph.width + px] * (dx ? sx - left : 1 - (sx - left)) *
                                 (dy ? sy - top : 1 - (sy - top));
                }
            const unsigned int nibble = std::clamp(static_cast<int>(std::lround(alpha / 17)), 0, 15);
            result.pixels[(y * 16 + x) / 2] |= nibble << ((x % 2 == 0) ? 4 : 0);
        }
    }
    return cells.emplace(codepoint, std::move(result)).first->second;
}

float UIFontGlyphs::Measure(std::string_view text) {
    if (text.empty())
        return 0;
    float line = 0, width = 0;
    const char* cursor = text.data();
    const char* end = cursor + text.size();
    while (cursor < end) {
        unsigned int codepoint;
        const int bytes = ImTextCharFromUtf8(&codepoint, cursor, end);
        if (bytes <= 0 || codepoint == 0)
            break;
        cursor += bytes;
        if (codepoint == '\n') {
            width = std::max(width, line);
            line = 0;
        } else if (codepoint != '\r') {
            line += Get(codepoint).advance;
        }
    }
    return std::max(width, line);
}

std::vector<UIFontGlyphs::Placement> UIFontGlyphs::Layout(std::string_view text) {
    std::vector<Placement> result;
    if (text.empty())
        return result;
    float x = 0;
    float y = 0;
    const char* cursor = text.data();
    const char* end = cursor + text.size();
    while (cursor < end) {
        unsigned int codepoint;
        const int bytes = ImTextCharFromUtf8(&codepoint, cursor, end);
        if (bytes <= 0 || codepoint == 0) {
            break;
        }
        cursor += bytes;
        if (codepoint == '\n') {
            x = 0;
            y += font->FontSize;
        } else if (codepoint != '\r') {
            const auto& glyph = Get(codepoint);
            result.push_back({ &glyph, x, y });
            x += glyph.advance;
        }
    }
    return result;
}
} // namespace Lang
