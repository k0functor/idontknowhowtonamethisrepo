#pragma once

#include "data/Json.hpp"
#include "run/RunState.hpp"

#include <filesystem>

class RunStateSerializer {
public:
    static Json toJson(const RunState& run);
    static RunState fromJson(const Json& json, const std::filesystem::path& sourcePath);
};
