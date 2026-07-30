#include "run/RunCompletion.hpp"

RunCompletionStatus RunCompletion::evaluate(const RunState& run, const FloorDatabase& floors) {
    RunCompletionStatus status;

    if (!run.actCompleted) {
        return status;
    }

    status.nextFloorId = run.nextFloorId;

    if (run.nextFloorId.empty()) {
        status.type = RunCompletionType::Victory;
        return status;
    }

    if (!floors.contains(run.nextFloorId) || !floors.get(run.nextFloorId).isImplemented) {
        status.type = RunCompletionType::PlayableContentComplete;
        return status;
    }

    status.type = RunCompletionType::FloorCleared;
    status.canContinueToNextFloor = true;
    return status;
}

bool RunCompletion::isFinal(const RunCompletionType type) {
    return type == RunCompletionType::PlayableContentComplete || type == RunCompletionType::Victory;
}
