#pragma once

#include "run/RunMapNode.hpp"
#include "run/RunState.hpp"

struct RewardContext {
    const RunState& run;
    RunMapNodeType nodeType = RunMapNodeType::Combat;
    int enemyCount = 1;
};
