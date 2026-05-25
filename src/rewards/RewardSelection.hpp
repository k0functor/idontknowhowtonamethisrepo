#pragma once

#include "cards/CardId.hpp"

#include <optional>

struct RewardSelection {
    bool takeGold = false;
    std::optional<CardId> selectedCardId;
    bool skippedCardReward = false;
};
