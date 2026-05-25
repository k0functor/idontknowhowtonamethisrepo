#pragma once

#include "rewards/RewardSelection.hpp"
#include "rewards/RewardState.hpp"
#include "run/RunState.hpp"

class RewardSystem {
public:
    void applyReward(
        RunState& run,
        const RewardState& reward,
        const RewardSelection& selection
    ) const;
};
