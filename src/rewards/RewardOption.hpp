#pragma once

#include "rewards/CardRewardOption.hpp"

#include <string>
#include <utility>
#include <vector>

enum class RewardOptionType {
    Gold,
    CardChoice,
    Consumable,
    Relic,
    ActiveItem
};

struct RewardOption {
    RewardOptionType type = RewardOptionType::Gold;

    int gold = 0;
    std::vector<CardRewardOption> cardOptions;
    std::string consumableId;
    std::string relicId;
    std::string activeItemId;

    static RewardOption goldReward(int amount) {
        RewardOption option;
        option.type = RewardOptionType::Gold;
        option.gold = amount;
        return option;
    }

    static RewardOption cardChoice(std::vector<CardRewardOption> options) {
        RewardOption option;
        option.type = RewardOptionType::CardChoice;
        option.cardOptions = std::move(options);
        return option;
    }

    static RewardOption consumable(std::string id) {
        RewardOption option;
        option.type = RewardOptionType::Consumable;
        option.consumableId = std::move(id);
        return option;
    }

    static RewardOption relic(std::string id) {
        RewardOption option;
        option.type = RewardOptionType::Relic;
        option.relicId = std::move(id);
        return option;
    }

    static RewardOption activeItem(std::string id) {
        RewardOption option;
        option.type = RewardOptionType::ActiveItem;
        option.activeItemId = std::move(id);
        return option;
    }
};
