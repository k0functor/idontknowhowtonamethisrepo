#pragma once

#include "core/Vec2.hpp"
#include "run/RunMapNodeType.hpp"

#include <string>
#include <vector>

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

    Vec2 position{0.f, 0.f};
    std::vector<int> nextNodeIds;
};
