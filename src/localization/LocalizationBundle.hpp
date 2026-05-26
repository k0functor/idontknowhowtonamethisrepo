#pragma once

#include "Locale.hpp"

#include <filesystem>
#include <string>
#include <unordered_map>

class LocalizationBundle {
public:
    explicit LocalizationBundle(Locale locale);

    const Locale& locale() const;

    // Loads either a single JSON object file or every *.json file from a directory.
    // Directory loading merges files and rejects duplicate text ids.
    void loadFromFile(const std::filesystem::path& filePath);

    bool contains(const std::string& textId) const;
    const std::string& get(const std::string& textId) const;

private:
    void mergeFileInto(
        const std::filesystem::path& filePath,
        std::unordered_map<std::string, std::string>& targetTexts
    ) const;

private:
    Locale locale_;
    std::unordered_map<std::string, std::string> texts_;
};
