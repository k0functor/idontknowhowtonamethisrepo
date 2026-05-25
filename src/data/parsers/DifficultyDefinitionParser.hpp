#pragma once

#include "data/Json.hpp"
#include "run/DifficultyDefinition.hpp"

#include <filesystem>

class DifficultyDefinitionParser {
public:
    static DifficultyDefinition parse(
        const Json& json,
        const std::filesystem::path& sourcePath
    );
};
