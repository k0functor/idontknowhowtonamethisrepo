#pragma once

#include "data/Json.hpp"
#include "enemies/EnemyActionDefinition.hpp"

#include <filesystem>

class EnemyActionDefinitionParser {
public:
    static EnemyActionDefinition parse(
        const Json& json,
        const std::filesystem::path& sourcePath
    );
};
