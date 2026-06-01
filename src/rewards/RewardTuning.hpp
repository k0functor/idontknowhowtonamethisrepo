#pragma once

#include "cards/CardRarity.hpp"
#include "run/RunMapNode.hpp"

#include <filesystem>
#include <optional>

struct NodeRewardTuning {
    int gold = 0;
    bool offerCards = false;
    int cardChoices = 0;
    bool guaranteedRelic = false;
    int consumableChancePercent = 0;
    std::optional<CardRarity> minimumCardRarity;
};

class RewardTuning {
public:
    RewardTuning();

    void loadFromFile(const std::filesystem::path& filePath);

    const NodeRewardTuning& node(RunMapNodeType nodeType) const;
    double merchantGoldMultiplier() const;

private:
    NodeRewardTuning& mutableNode(RunMapNodeType nodeType);

private:
    NodeRewardTuning combat_;
    NodeRewardTuning elite_;
    NodeRewardTuning event_;
    NodeRewardTuning shop_;
    NodeRewardTuning chest_;
    NodeRewardTuning rest_;
    NodeRewardTuning boss_;
    double merchantGoldMultiplier_ = 1.25;
};
