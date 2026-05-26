#pragma once

#include "rewards/RewardOption.hpp"
#include "run/RunMapNode.hpp"

#include <vector>

struct RewardState {
    RunMapNodeType sourceNodeType = RunMapNodeType::Combat;
    std::vector<RewardOption> options;

    bool empty() const {
        return options.empty();
    }
};
