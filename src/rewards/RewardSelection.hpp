#pragma once

#include "cards/CardId.hpp"

#include <string>
#include <vector>

struct RewardSelection {
    int goldTaken = 0;
    std::vector<CardId> selectedCardIds;
    std::vector<std::string> selectedConsumableIds;

    bool empty() const {
        return goldTaken == 0 && selectedCardIds.empty() && selectedConsumableIds.empty();
    }
};
