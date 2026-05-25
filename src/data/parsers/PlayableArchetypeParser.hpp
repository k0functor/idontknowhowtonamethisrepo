#pragma once

#include "archetypes/PlayableArchetypeDefinition.hpp"
#include "data/Json.hpp"

#include <filesystem>

class PlayableArchetypeParser {
public:
    static PlayableArchetypeDefinition parse(
        const Json& json,
        const std::filesystem::path& sourcePath
    );
};
