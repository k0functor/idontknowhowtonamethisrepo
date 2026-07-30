#pragma once

#include "challenges/ChallengeDefinition.hpp"
#include "data/Json.hpp"

#include <filesystem>

class ChallengeDefinitionParser {
public:
    static ChallengeDefinition parse(
        const Json& json,
        const std::filesystem::path& sourcePath
    );
};
