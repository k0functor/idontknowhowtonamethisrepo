#include "LocalizationBundle.hpp"

#include "data/JsonLoader.hpp"

#include <filesystem>
#include <stdexcept>
#include <utility>

LocalizationBundle::LocalizationBundle(Locale locale) : locale_(std::move(locale)) {}

const Locale& LocalizationBundle::locale() const {
    return locale_;
}

void LocalizationBundle::loadFromFile(const std::filesystem::path& filePath) {
    const Json root = JsonLoader::loadObjectFromFile(filePath);

    std::unordered_map<std::string, std::string> loadedTexts;
    loadedTexts.reserve(root.size());

    for (const auto& [textId, value] : root.items()) {
        if (textId.empty()) {
            throw std::runtime_error(filePath.string() + ": localization text id cannot be empty");
        }

        if (!value.is_string()) {
            throw std::runtime_error(
                filePath.string() + ": localization value for '" + textId + "' must be a string"
            );
        }

        loadedTexts.emplace(textId, value.get<std::string>());
    }

    texts_ = std::move(loadedTexts);
}

bool LocalizationBundle::contains(const std::string& textId) const {
    return texts_.contains(textId);
}

const std::string& LocalizationBundle::get(const std::string& textId) const {
    const auto iterator = texts_.find(textId);

    if (iterator == texts_.end()) {
        throw std::runtime_error(
            "Missing localization text '" + textId + "' for locale '" + locale_.code() + "'"
        );
    }

    return iterator->second;
}

