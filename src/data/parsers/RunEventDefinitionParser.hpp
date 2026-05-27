#pragma once

#include "data/Json.hpp"
#include "events/RunEventDefinition.hpp"

#include <filesystem>

class RunEventDefinitionParser {
public:
    static RunEventDefinition parse(
        const Json& json,
        const std::filesystem::path& sourcePath
    );
};
