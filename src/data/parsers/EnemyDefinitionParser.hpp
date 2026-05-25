#pragma once

#include "data/Json.hpp"
#include "enemies/EnemyDefinition.hpp"

#include <filesystem>

class EnemyDefinitionParser {
public:
    static EnemyDefinition parse(
        const Json& json,
        const std::filesystem::path& sourcePath
    );
};
