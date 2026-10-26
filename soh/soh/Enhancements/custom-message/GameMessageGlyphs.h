#pragma once
#include <stdint.h>
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif
// Only active for a custom UTF-8 buffer. Native Japanese ROM text stays on its
// existing decoder. Each page owns advance metadata indexed by decoded position.
void GameMessage_BeginPage(void);
// Returns zero for native Latin/button glyphs; the caller uses Font_LoadChar.
size_t GameMessage_DecodeGlyph(const char* input, size_t length, uint8_t* texture, uint16_t decodedIndex);
float GameMessage_GetAdvance(uint16_t decodedIndex, float legacyAdvance);
int16_t GameMessage_GetDrawOffset(uint16_t decodedIndex);
float GameMessage_GetScaledAdvance(uint16_t decodedIndex, float legacyAdvance, float scale);
// Measure the fully decoded page once, after names/numbers are substituted.
// Only Japanese custom UTF-8 pages without an item icon receive alignment.
void GameMessage_AlignPage(const uint8_t* decoded, size_t length, const float* legacyWidths, size_t legacyWidthCount,
                           float scale, int16_t spaceAdvance, int16_t baseX);
int16_t GameMessage_GetLineStart(uint16_t decodedIndex, int16_t legacyX);
int16_t GameMessage_GetChoiceX(uint8_t choiceIndex, int16_t legacyX);
#ifdef __cplusplus
}
#endif
