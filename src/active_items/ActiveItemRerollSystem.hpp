#pragma once

#include "active_items/ActiveItemDatabase.hpp"
#include "consumables/ConsumableDatabase.hpp"
#include "core/Random.hpp"
#include "data/CardDatabase.hpp"
#include "relics/RelicDatabase.hpp"
#include "relics/RelicRarity.hpp"
#include "rewards/RewardState.hpp"
#include "rewards/RewardTuning.hpp"
#include "run/RunState.hpp"
#include "shop/ShopState.hpp"

#include <functional>

struct ActiveItemRerollResult {
    int cardOffersChanged = 0;
    int relicOffersChanged = 0;
    int consumableOffersChanged = 0;
    int activeItemOffersChanged = 0;

    int totalChanged() const {
        return cardOffersChanged + relicOffersChanged + consumableOffersChanged + activeItemOffersChanged;
    }

    bool changed() const {
        return totalChanged() > 0;
    }
};

class ActiveItemRerollSystem {
public:
    static ActiveItemRerollResult rerollReward(
        RewardState& reward,
        const RunState& run,
        const CardDatabase& cards,
        const RelicDatabase& relics,
        const ConsumableDatabase& consumables,
        const ActiveItemDatabase& activeItems,
        const NodeRewardTuning& tuning,
        Random& random
    );

    static ActiveItemRerollResult rerollShop(
        ShopState& shop,
        const RunState& run,
        const CardDatabase& cards,
        const RelicDatabase& relics,
        const ConsumableDatabase& consumables,
        const ActiveItemDatabase& activeItems,
        int minimumCardPrice,
        int minimumConsumablePrice,
        const std::function<int(RelicRarity)>& relicPrice,
        Random& random
    );
};
