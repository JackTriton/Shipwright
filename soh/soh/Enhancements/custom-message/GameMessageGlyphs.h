#pragma once
#include <stdint.h>
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif
// Only active for a custom UTF-8 buffer. Native Japanese ROM text stays on its
// existing decoder. Each page owns advance metadata indexed by decoded position.
void GameMessage_BeginPage(void);
size_t GameMessage_DecodeGlyph(const char* input, size_t length, uint8_t* texture, uint16_t decodedIndex);
float GameMessage_GetAdvance(uint16_t decodedIndex, float legacyAdvance);
int16_t GameMessage_GetDrawOffset(uint16_t decodedIndex);
float GameMessage_GetScaledAdvance(uint16_t decodedIndex, float legacyAdvance, float scale);
#ifdef __cplusplus
}
#endif
