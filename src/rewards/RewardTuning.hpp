#pragma once

#include "run/RunMapNode.hpp"

#include <filesystem>

struct NodeRewardTuning {
    int gold = 0;
    bool offerCards = false;
    int cardChoices = 0;
    bool guaranteedRelic = false;
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
