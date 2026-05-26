#include "LocalizationBundle.hpp"

#include "data/JsonLoader.hpp"

#include <algorithm>
#include <filesystem>
#include <stdexcept>
#include <utility>
#include <vector>

LocalizationBundle::LocalizationBundle(Locale locale) : locale_(std::move(locale)) {}

const Locale& LocalizationBundle::locale() const {
    return locale_;
}

void LocalizationBundle::loadFromFile(const std::filesystem::path& filePath) {
    std::unordered_map<std::string, std::string> loadedTexts;

    if (std::filesystem::is_directory(filePath)) {
        std::vector<std::filesystem::path> files;

        for (const std::filesystem::directory_entry& entry : std::filesystem::directory_iterator(filePath)) {
            if (!entry.is_regular_file()) {
                continue;
            }

            if (entry.path().extension() == ".json") {
                files.push_back(entry.path());
            }
        }

        std::sort(files.begin(), files.end());

        if (files.empty()) {
            throw std::runtime_error(
                "Localization directory '" + filePath.string() + "' does not contain any .json files"
            );
        }

        for (const std::filesystem::path& file : files) {
            mergeFileInto(file, loadedTexts);
        }
    } else {
        mergeFileInto(filePath, loadedTexts);
    }

    texts_ = std::move(loadedTexts);
}

void LocalizationBundle::mergeFileInto(
    const std::filesystem::path& filePath,
    std::unordered_map<std::string, std::string>& targetTexts
) const {
    const Json root = JsonLoader::loadObjectFromFile(filePath);

    for (const auto& [textId, value] : root.items()) {
        if (textId.empty()) {
            throw std::runtime_error(filePath.string() + ": localization text id cannot be empty");
        }

        if (!value.is_string()) {
            throw std::runtime_error(
                filePath.string() + ": localization value for '" + textId + "' must be a string"
            );
        }

        const auto [iterator, inserted] = targetTexts.emplace(textId, value.get<std::string>());

        if (!inserted) {
            throw std::runtime_error(
                filePath.string() + ": duplicate localization text id '" + textId + "' while loading locale '" +
                locale_.code() + "'"
            );
        }
    }
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
