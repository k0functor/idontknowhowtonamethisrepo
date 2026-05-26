#pragma once

#include "consumables/ConsumableDefinition.hpp"
#include "data/Json.hpp"

#include <filesystem>

class ConsumableDefinitionParser {
public:
    static ConsumableDefinition parse(
        const Json& json,
        const std::filesystem::path& sourcePath
    );
};
