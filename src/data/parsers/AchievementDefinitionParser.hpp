#pragma once

#include "achievements/AchievementDefinition.hpp"
#include "data/Json.hpp"

#include <filesystem>

class AchievementDefinitionParser {
public:
    static AchievementDefinition parse(
        const Json& json,
        const std::filesystem::path& sourcePath
    );
};
