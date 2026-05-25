#pragma once

#include "core/Random.hpp"
#include "data/CardDatabase.hpp"
#include "rewards/RewardContext.hpp"
#include "rewards/RewardState.hpp"

class RewardGenerator {
public:
    RewardState generateCombatReward(
        const RewardContext& context,
        const CardDatabase& cards,
        Random& random
    ) const;

private:
    int baseGoldForNode(RunMapNodeType nodeType) const;
    bool shouldOfferCards(const RewardContext& context) const;
    int cardRewardCount(const RewardContext& context) const;
};
