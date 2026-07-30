#pragma once

#include "run/FloorDatabase.hpp"
#include "run/RunCompletionType.hpp"
#include "run/RunState.hpp"

#include <string>

struct RunCompletionStatus {
    RunCompletionType type = RunCompletionType::InProgress;
    std::string nextFloorId;
    bool canContinueToNextFloor = false;
};

class RunCompletion {
public:
    static RunCompletionStatus evaluate(const RunState& run, const FloorDatabase& floors);
    static bool isFinal(RunCompletionType type);
};
