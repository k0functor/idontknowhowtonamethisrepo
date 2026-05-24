#include "LocalizationManager.hpp"

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
    if (!hasLocale(currentLocale_)) {
        return false;
    }

    return currentBundle().contains(textId.value);
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