#pragma once

#include "core/Random.hpp"
#include "data/CardDatabase.hpp"
#include "rewards/RewardContext.hpp"
#include "rewards/RewardState.hpp"
#include "relics/RelicDatabase.hpp"

class RewardGenerator {
public:
    RewardState generateCombatReward(
        const RewardContext& context,
        const CardDatabase& cards,
        const RelicDatabase& relics,
        Random& random
    ) const;

private:
    int baseGoldForNode(RunMapNodeType nodeType) const;
    bool shouldOfferCards(const RewardContext& context) const;
    double relicGoldMultiplier(const RewardContext& context, const RelicDatabase& relics) const;
    int cardRewardCount(const RewardContext& context) const;
};
