#pragma once

#include "core/Random.hpp"
#include "data/CardDatabase.hpp"
#include "rewards/RewardContext.hpp"
#include "rewards/RewardState.hpp"
#include "rewards/RewardTuning.hpp"
#include "relics/RelicDatabase.hpp"
#include "relics/RelicId.hpp"

#include <optional>

class RewardGenerator {
public:
    RewardState generateCombatReward(
        const RewardContext& context,
        const CardDatabase& cards,
        const RelicDatabase& relics,
        const RewardTuning& tuning,
        Random& random
    ) const;

private:
    bool shouldOfferCards(const RewardContext& context, const RewardTuning& tuning) const;
    double relicGoldMultiplier(const RewardContext& context, const RelicDatabase& relics) const;
    int cardRewardCount(const RewardContext& context, const RewardTuning& tuning) const;
    std::optional<RelicId> chooseRelicReward(
        const RewardContext& context,
        const RelicDatabase& relics,
        Random& random
    ) const;
};
