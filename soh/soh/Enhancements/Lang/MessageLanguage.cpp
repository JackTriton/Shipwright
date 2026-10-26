#include "MessageLanguage.h"
#include "LanguagePolicy.h"

namespace {
uint8_t requestedLanguage = 0;
uint8_t actualLanguage = 0;
bool explicitlySet = false;
bool unicode = false;
} // namespace

void MessageLanguage_Begin(int32_t language) {
    requestedLanguage = actualLanguage = LanguagePolicy_Normalize(language);
    explicitlySet = false;
    unicode = false;
}

void MessageLanguage_Set(int32_t language) {
    unicode = false;
    actualLanguage = LanguagePolicy_Normalize(language);
    explicitlySet = true;
}

void MessageLanguage_SetUnicode(int32_t language) {
    MessageLanguage_Set(language);
    unicode = true;
}
bool MessageLanguage_UsesUnicode(void) {
    return unicode;
}

void MessageLanguage_UseLegacyFallback(void) {
    if (!explicitlySet) {
        MessageLanguage_Set(LanguagePolicy_LegacyText(requestedLanguage));
    }
}

uint8_t MessageLanguage_Get(void) {
    return actualLanguage;
}

bool MessageLanguage_IsJapanese(void) {
    return actualLanguage == 3 && !unicode;
}
bool MessageLanguage_UsesJapaneseLayout(void) {
    return actualLanguage == 3;
}

bool MessageLanguage_IsEnglishFallback(void) {
    return !unicode && requestedLanguage == 3 && actualLanguage == 0;
}
