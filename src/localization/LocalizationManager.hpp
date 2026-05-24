#pragma once

#include "Locale.hpp"
#include "LocalizationBundle.hpp"
#include "MissingTextPolicy.hpp"
#include "TextFormatter.hpp"
#include "TextId.hpp"

#include <filesystem>
#include <string>
#include <unordered_map>
#include <cstddef>

class LocalizationManager {
public:
    void setMissingTextPolicy(MissingTextPolicy policy);

    void loadBundle(
        const Locale& locale,
        const std::filesystem::path& filePath
    );

    void setCurrentLocale(const Locale& locale);
    const Locale& currentLocale() const;

    bool hasLocale(const Locale& locale) const;
    bool hasText(const TextId& textId) const;

    std::string get(const TextId& textId) const;

    std::string format(
        const TextId& textId,
        const TextFormatter::Variables& variables
    ) const;

private:
    const LocalizationBundle& currentBundle() const;

private:
    struct LocaleHash {
        std::size_t operator()(const std::string& value) const {
            return std::hash<std::string>{}(value);
        }
    };

    std::unordered_map<std::string, LocalizationBundle> bundles_;

    Locale currentLocale_ = Locale::russian();
    MissingTextPolicy missingTextPolicy_ = MissingTextPolicy::Throw;
};