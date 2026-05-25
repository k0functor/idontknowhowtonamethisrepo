#pragma once

#include "data/Json.hpp"
#include "relics/RelicDefinition.hpp"

#include <filesystem>

class RelicDefinitionParser {
public:
    static RelicDefinition parse(
        const Json& json,
        const std::filesystem::path& sourcePath
    );
};
