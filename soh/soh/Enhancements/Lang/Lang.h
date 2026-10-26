#pragma once

#include <cstddef>
#include <functional>
#include <map>
#include <stdint.h>
#include <string>
#include <variant>

namespace Lang {

class TranslationCatalog;

enum Error {
    LanguageDataNotInitialized,
    LanguageDataNotFound,
    PathDataNotFound,
    PathInvalidValue,
};

// TODO: Upgrade to std::expected when we update to C++ 23
std::variant<std::reference_wrapper<const std::string>, Lang::Error> TryTranslate(const char* path);
std::variant<std::reference_wrapper<const std::string>, Lang::Error> TryTranslateForLanguage(const char* path,
                                                                                             const char* language);
const std::string& Translate(const char* path);
// Use an explicit locale for data formats whose names must not follow the UI.
const std::string& TranslateForLanguage(const char* path, const char* language);
const char* GetUILanguage();
uint64_t GetUICatalogRevision();
bool SetUILanguage(const std::string& language);
const std::map<std::string, std::string>& GetAvailableUILanguages();
uint8_t GetUIResourceLanguage();
uint8_t GetUILegacyTextLanguage();
std::string TranslateOrFallback(const std::string& path, const std::string& fallback);
void LoadLangs();
// Load catalogs before menu/font initialization; saved selection is applied by
// LoadLangs after the normal configuration migration. Both operations are idempotent.
void PrepareUICatalogs();
bool IsUsingBuiltinUICatalog();
// Reuses the prepared catalog; never reloads resources or changes selections.
std::string GetUIFontText();
// Loads direct *.json children through the existing archive/resource manager.
// Call only at startup, before any catalog readers are running.
struct CatalogLoadResult {
    std::size_t loaded = 0;
    bool hasEnglish = false;
};
CatalogLoadResult LoadCatalogs(TranslationCatalog& catalog, const std::string& folder);
} // namespace Lang
