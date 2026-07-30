#pragma once

#include "run/RunEndReason.hpp"
#include "run/RunState.hpp"

#include <filesystem>

class RunTelemetryWriter {
public:
    static void append(const std::filesystem::path& path, const RunState& run, RunEndReason reason);
};
