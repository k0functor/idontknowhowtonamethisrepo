#include "LocalizationManager.hpp"

#include <algorithm>
#include <iterator>
#include <sstream>
#include <stdexcept>

void LocalizationManager::setMissingTextPolicy(const MissingTextPolicy policy) {
    missingTextPolicy_ = policy;
}

void LocalizationManager::loadBundle(
    const Locale& locale,
    const std::filesystem::path& filePath
) {
    LocalizationBundle bundle(locale);
    bundle.loadFromFile(filePath);

    bundles_.insert_or_assign(locale.code(), std::move(bundle));
}

void LocalizationManager::setCurrentLocale(const Locale& locale) {
    if (!hasLocale(locale)) {
        throw std::runtime_error(
            "Cannot set current locale to '" + locale.code() +
            "': locale is not loaded"
        );
    }

    currentLocale_ = locale;
}

const Locale& LocalizationManager::currentLocale() const {
    return currentLocale_;
}

bool LocalizationManager::hasLocale(const Locale& locale) const {
    return bundles_.contains(locale.code());
}

bool LocalizationManager::hasText(const TextId& textId) const {
    return hasText(currentLocale_, textId);
}

bool LocalizationManager::hasText(const Locale& locale, const TextId& textId) const {
    const auto iterator = bundles_.find(locale.code());
    if (iterator == bundles_.end()) {
        return false;
    }

    return iterator->second.contains(textId.value);
}

std::vector<std::string> LocalizationManager::textIds(const Locale& locale) const {
    const auto iterator = bundles_.find(locale.code());
    if (iterator == bundles_.end()) {
        throw std::runtime_error("Locale '" + locale.code() + "' is not loaded");
    }

    return iterator->second.textIds();
}

void LocalizationManager::validateAllLocalesHaveSameTextIds() const {
    if (bundles_.size() < 2) {
        return;
    }

    const auto referenceIterator = bundles_.begin();
    const std::string referenceLocale = referenceIterator->first;
    const std::vector<std::string> referenceIds = referenceIterator->second.textIds();

    std::vector<std::string> errors;

    for (const auto& [localeCode, bundle] : bundles_) {
        if (localeCode == referenceLocale) {
            continue;
        }

        const std::vector<std::string> ids = bundle.textIds();

        std::vector<std::string> missing;
        std::set_difference(
            referenceIds.begin(), referenceIds.end(),
            ids.begin(), ids.end(),
            std::back_inserter(missing)
        );

        std::vector<std::string> extra;
        std::set_difference(
            ids.begin(), ids.end(),
            referenceIds.begin(), referenceIds.end(),
            std::back_inserter(extra)
        );

        for (const std::string& textId : missing) {
            errors.push_back(
                "Locale '" + localeCode + "' is missing text id '" + textId +
                "' present in '" + referenceLocale + "'"
            );
        }

        for (const std::string& textId : extra) {
            errors.push_back(
                "Locale '" + localeCode + "' has extra text id '" + textId +
                "' absent in '" + referenceLocale + "'"
            );
        }
    }

    if (!errors.empty()) {
        std::ostringstream out;
        out << "Localization validation failed with " << errors.size() << " error(s):";
        for (const std::string& error : errors) {
            out << "\n - " << error;
        }
        throw std::runtime_error(out.str());
    }
}

std::string LocalizationManager::get(const TextId& textId) const {
    if (!hasLocale(currentLocale_)) {
        throw std::runtime_error(
            "Current locale '" + currentLocale_.code() + "' is not loaded"
        );
    }

    const LocalizationBundle& bundle = currentBundle();

    if (bundle.contains(textId.value)) {
        return bundle.get(textId.value);
    }

    if (missingTextPolicy_ == MissingTextPolicy::ShowTextId) {
        return textId.value;
    }

    throw std::runtime_error(
        "Missing localization text '" + textId.value +
        "' for locale '" + currentLocale_.code() + "'"
    );
}

std::string LocalizationManager::format(
    const TextId& textId,
    const TextFormatter::Variables& variables
) const {
    return TextFormatter::format(get(textId), variables);
}

const LocalizationBundle& LocalizationManager::currentBundle() const {
    const auto iterator = bundles_.find(currentLocale_.code());

    if (iterator == bundles_.end()) {
        throw std::runtime_error(
            "Locale '" + currentLocale_.code() + "' is not loaded"
        );
    }

    return iterator->second;
}