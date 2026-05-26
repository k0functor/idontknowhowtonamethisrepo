#pragma once

#include "data/Json.hpp"
#include "drones/DroneDefinition.hpp"

#include <filesystem>

class DroneDefinitionParser {
public:
    static DroneDefinition parse(
        const Json& json,
        const std::filesystem::path& sourcePath
    );
};
