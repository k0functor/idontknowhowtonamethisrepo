#pragma once

#include "data/Json.hpp"
#include "effects/EffectDefinition.hpp"

#include <filesystem>

class EffectDefinitionParser {
public:
    static EffectDefinition parse(
        const Json& json,
        const std::filesystem::path& sourcePath
    );
};
