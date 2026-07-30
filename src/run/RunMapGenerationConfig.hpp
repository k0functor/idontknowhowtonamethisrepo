#pragma once

#include <filesystem>
#include <string>
#include <vector>

struct RunMapLayoutConfig {
    float startX = 120.f;
    float layerStepX = 210.f;
    float centerY = 520.f;
    float nodeSpacingY = 170.f;
};

struct RunMapPlacementRules {
    int maxPerLayer = 0;
    int minLayerGap = 0;
};

struct RunMapSpecialNodeConfig {
    int count = 0;
    int minLayer = 1;
    int maxLayer = 1;
    bool fullLayer = false;
    RunMapPlacementRules placement;
};

struct RunMapEliteConfig {
    int minimum = 1;
    int maximum = 2;
    int minLayer = 1;
    int maxLayer = 1;
    RunMapPlacementRules placement;
};

struct RunMapEventConfig {
    int minimum = 0;
    int maximum = 0;
    int minLayer = 1;
    int maxLayer = 1;
    RunMapPlacementRules placement;
};

class RunMapGenerationConfig {
public:
    void loadFromFile(const std::filesystem::path& filePath);

    const std::string& id() const;
    int layerCount() const;
    int middleMinNodes() const;
    int middleMaxNodes() const;
    bool hasLayerNodeCounts() const;
    int nodeCountForLayer(int layer, class Random& random) const;
    const std::vector<int>& layerNodeCounts() const;
    int combatWeight() const;
    int eventWeight() const;
    int extraConnectionChance() const;
    int questionMarkCombatChance() const;
    const RunMapSpecialNodeConfig& shop() const;
    const RunMapSpecialNodeConfig& chests() const;
    const RunMapEliteConfig& elites() const;
    const RunMapEventConfig& events() const;
    bool hasFixedEvents() const;
    const RunMapLayoutConfig& layout() const;

private:
    void validate(const std::filesystem::path& filePath) const;

private:
    std::string id_ = "act1";
    int layerCount_ = 12;
    int middleMinNodes_ = 2;
    int middleMaxNodes_ = 4;
    std::vector<int> layerNodeCounts_;
    int combatWeight_ = 70;
    int eventWeight_ = 30;
    int extraConnectionChance_ = 35;
    int questionMarkCombatChance_ = 30;
    RunMapSpecialNodeConfig shop_{1, 2, 8, false, RunMapPlacementRules{}};
    RunMapSpecialNodeConfig chests_{0, 1, 1, false, RunMapPlacementRules{}};
    RunMapEliteConfig elites_{1, 2, 3, 8, RunMapPlacementRules{}};
    RunMapEventConfig events_{0, 0, 1, 1, RunMapPlacementRules{}};
    bool hasFixedEvents_ = false;
    RunMapLayoutConfig layout_;
};
