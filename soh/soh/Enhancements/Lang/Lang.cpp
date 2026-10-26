#include <map>
#include <mutex>
#include <set>
#include <nlohmann/json.hpp>

#include <libultraship/bridge/consolevariablebridge.h>

#include "Lang.h"
#include "BuiltinUICatalog.h"
#include "TranslationCatalog.h"
#include "LanguagePolicy.h"
#include "soh/cvar_prefixes.h"
#include "spdlog/spdlog.h"

namespace {

bool initialized = false;
bool selectionInitialized = false;
bool usingBuiltinCatalog = false;
std::mutex diagnosticsMutex;
std::set<std::pair<std::string, std::string>> reportedMissing;
Lang::TranslationCatalog catalog;
std::map<std::string, std::string> languageNames;

constexpr const char* LANGUAGE_CVAR = CVAR_SETTING("UILanguage");
constexpr const char* LEGACY_LANGUAGE_CVAR = CVAR_SETTING("Language");
constexpr const char* DEFAULT_LANGUAGE = "en_US";
constexpr const char* UI_CATALOG_FOLDER = "ui-lang";
const std::string errorMessage =
    "[ERROR] Couldn't retrieve language data for this item, check the log for more information.";

} // namespace

std::variant<std::reference_wrapper<const std::string>, Lang::Error> Lang::TryTranslate(const char* path) {
    return TryTranslateForLanguage(path, GetUILanguage());
}

std::variant<std::reference_wrapper<const std::string>, Lang::Error>
Lang::TryTranslateForLanguage(const char* path, const char* language) {
    if (!initialized) {
        return Lang::Error::LanguageDataNotInitialized;
    }
    if (!catalog.HasLanguage(DEFAULT_LANGUAGE)) {
        return Lang::Error::LanguageDataNotFound;
    }
    if (path == nullptr || language == nullptr) {
        return Lang::Error::PathInvalidValue;
    }
    if (const auto* text = catalog.Find(language, path)) {
        return std::cref(*text);
    }
    return Lang::Error::PathDataNotFound;
}

const std::string& Lang::Translate(const char* path) {
    return TranslateForLanguage(path, GetUILanguage());
}

const std::string& Lang::TranslateForLanguage(const char* path, const char* language) {
    auto value = TryTranslateForLanguage(path, language);
    if (std::holds_alternative<std::reference_wrapper<const std::string>>(value)) {
        return std::get<std::reference_wrapper<const std::string>>(value);
    }
    // A missing translation may be requested once per frame. Report each pair
    // once, without making optional TryTranslate lookups produce any I/O.
    const std::lock_guard<std::mutex> lock(diagnosticsMutex);
    if (reportedMissing.emplace(language ? language : "<null>", path ? path : "<null>").second) {
        SPDLOG_WARN("Couldn't retrieve language data for locale ({}) at path ({})", language ? language : "<null>",
                    path ? path : "<null>");
    }
    return errorMessage;
}

void Lang::PrepareUICatalogs() {
    if (initialized) {
        return;
    }
    catalog.AddLanguage(DEFAULT_LANGUAGE, GetBuiltinUICatalog());
    const auto result = LoadCatalogs(catalog, UI_CATALOG_FOLDER);
    usingBuiltinCatalog = !result.hasEnglish;
    for (const auto& language : catalog.GetLanguages()) {
        if (const auto* name = catalog.FindExact(language, "language_name")) {
            languageNames.emplace(language, *name);
        }
    }
    initialized = true;
}

bool Lang::IsUsingBuiltinUICatalog() {
    return usingBuiltinCatalog;
}

void Lang::LoadLangs() {
    PrepareUICatalogs();
    if (selectionInitialized) {
        return;
    }
    selectionInitialized = true;
    // Initialize() has installed the application's file logger by this point.
    if (usingBuiltinCatalog) {
        SPDLOG_WARN("No valid ui-lang/en_US.json in mounted archives; using built-in English UI. "
                    "Rebuild GenerateSohOtr and deploy the matching soh.o2r beside the executable.");
    }
    const std::string configured = CVarGetString(LANGUAGE_CVAR, "");
    const std::string validLanguage =
        catalog.ResolveLanguage(configured, CVarGetString(LEGACY_LANGUAGE_CVAR, DEFAULT_LANGUAGE));
    if (configured != validLanguage && !usingBuiltinCatalog) {
        CVarSetString(LANGUAGE_CVAR, validLanguage.c_str());
        CVarSave();
    }
}

std::string Lang::GetUIFontText() {
    PrepareUICatalogs();
    return catalog.GetFontText();
}

const char* Lang::GetUILanguage() {
    if (!initialized) {
        return DEFAULT_LANGUAGE;
    }
    const char* language = CVarGetString(LANGUAGE_CVAR, DEFAULT_LANGUAGE);
    return catalog.HasLanguage(language) ? language : DEFAULT_LANGUAGE;
}

uint64_t Lang::GetUICatalogRevision() {
    return catalog.GetRevision();
}

bool Lang::SetUILanguage(const std::string& language) {
    if (!initialized || !catalog.HasLanguage(language)) {
        return false;
    }
    if (language != GetUILanguage()) {
        CVarSetString(LANGUAGE_CVAR, language.c_str());
        CVarSave();
    }
    return true;
}

const std::map<std::string, std::string>& Lang::GetAvailableUILanguages() {
    return languageNames;
}

uint8_t Lang::GetUIResourceLanguage() {
    return LanguagePolicy_FromLocale(GetUILanguage());
}

uint8_t Lang::GetUILegacyTextLanguage() {
    return LanguagePolicy_LegacyText(GetUIResourceLanguage());
}

std::string Lang::TranslateOrFallback(const std::string& path, const std::string& fallback) {
    const auto result = TryTranslate(path.c_str());
    if (const auto* text = std::get_if<std::reference_wrapper<const std::string>>(&result)) {
        return text->get();
    }
    return fallback;
}
