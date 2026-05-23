#pragma once

#include "Json.hpp"

#include <filesystem>

class JsonLoader {
public:
    static Json loadFromFile(const std::filesystem::path& filePath);

    static Json loadObjectFromFile(const std::filesystem::path& filePath);
    static Json loadArrayFromFile(const std::filesystem::path& filePath);
};