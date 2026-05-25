#pragma once

#include "actors/PlayerActorDefinition.hpp"
#include "data/Json.hpp"

#include <filesystem>

class PlayerActorParser {
public:
    static PlayerActorDefinition parse(
        const Json& json,
        const std::filesystem::path& sourcePath
    );
};
