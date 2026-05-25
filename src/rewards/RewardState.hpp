#pragma once

#include "rewards/CardRewardOption.hpp"
#include "run/RunMapNode.hpp"

#include <vector>

struct RewardState {
    int gold = 0;
    RunMapNodeType sourceNodeType = RunMapNodeType::Combat;
    std::vector<CardRewardOption> cardOptions;

    bool empty() const {
        return gold == 0 && cardOptions.empty();
    }
};
