#pragma once

#include "data/Json.hpp"
#include "statuses/StatusDefinition.hpp"

#include <filesystem>

class StatusDefinitionParser {
public:
    static StatusDefinition parse(
        const Json& json,
        const std::filesystem::path& sourcePath
    );
};
