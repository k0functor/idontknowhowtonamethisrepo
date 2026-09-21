#pragma once

#include "run/RunMapNodeType.hpp"

struct RunState;

class RunPacingTracker {
public:
    static void tick(RunState& run, float deltaSeconds);
    static void beginRoom(RunState& run, int nodeId, RunMapNodeType nodeType);
    static void finishRoom(RunState& run, int nodeId);
    static void finishFloor(RunState& run);
    static void beginFloor(RunState& run);
};
