#pragma once

#include "Locale.hpp"

#include <filesystem>
#include <string>
#include <unordered_map>

class LocalizationBundle {
public:
    explicit LocalizationBundle(Locale locale);

    const Locale& locale() const;

    void loadFromFile(const std::filesystem::path& filePath);

    bool contains(const std::string& textId) const;
    const std::string& get(const std::string& textId) const;

private:
    Locale locale_;
    std::unordered_map<std::string, std::string> texts_;
};