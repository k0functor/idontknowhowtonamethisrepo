#pragma once

#include "data/Json.hpp"
#include "effects/EffectValue.hpp"

#include <filesystem>

class EffectValueParser {
public:
    static EffectValue parse(
        const Json& json,
        const std::filesystem::path& sourcePath
    );
};
