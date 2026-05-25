#pragma once

#include "cards/CardDefinition.hpp"
#include "data/Json.hpp"

#include <filesystem>

class CardDefinitionParser {
public:
    static CardDefinition parse(
        const Json& json,
        const std::filesystem::path& sourcePath
    );
};
