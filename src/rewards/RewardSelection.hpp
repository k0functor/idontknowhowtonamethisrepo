#pragma once

#include "cards/CardId.hpp"

#include <string>
#include <vector>

struct RelicRewardSelection {
    std::string relicId;
    std::string actorDefinitionId;
};

struct RewardSelection {
    int goldTaken = 0;
    std::vector<CardId> selectedCardIds;
    std::vector<std::string> selectedConsumableIds;

    // Legacy/default-owner path. New multi-actor rewards should use
    // selectedRelics so the relic can be bound to a specific actor.
    std::vector<std::string> selectedRelicIds;
    std::vector<RelicRewardSelection> selectedRelics;

    bool empty() const {
        return goldTaken == 0 &&
            selectedCardIds.empty() &&
            selectedConsumableIds.empty() &&
            selectedRelicIds.empty() &&
            selectedRelics.empty();
    }
};
