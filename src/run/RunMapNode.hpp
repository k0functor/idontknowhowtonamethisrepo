#pragma once

#include <string>
#include <vector>

#include <raylib.h>

enum class RunMapNodeType {
    Combat,
    Elite,
    Event,
    Shop,
    Rest,
    Boss
};

enum class RunMapNodeState {
    Locked,
    Available,
    Completed,
    Current
};

struct RunMapNode {
    int id = 0;
    RunMapNodeType type = RunMapNodeType::Combat;
    RunMapNodeState state = RunMapNodeState::Locked;

    Vector2 position{0.f, 0.f};
    std::vector<int> nextNodeIds;
};

std::string toString(RunMapNodeType type);
