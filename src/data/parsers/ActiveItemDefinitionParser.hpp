#pragma once

#include "active_items/ActiveItemDefinition.hpp"
#include "data/Json.hpp"

#include <filesystem>

class ActiveItemDefinitionParser {
public:
    static ActiveItemDefinition parse(const Json& json, const std::filesystem::path& sourcePath);
};
