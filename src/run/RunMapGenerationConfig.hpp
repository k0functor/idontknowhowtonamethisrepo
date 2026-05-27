#pragma once

#include <filesystem>
#include <string>

struct RunMapLayoutConfig {
    float startX = 120.f;
    float layerStepX = 210.f;
    float centerY = 520.f;
    float nodeSpacingY = 170.f;
};

struct RunMapSpecialNodeConfig {
    int count = 0;
    int minLayer = 1;
    int maxLayer = 1;
};

struct RunMapEliteConfig {
    int minimum = 1;
    int maximum = 2;
    int minLayer = 1;
    int maxLayer = 1;
};

class RunMapGenerationConfig {
public:
    void loadFromFile(const std::filesystem::path& filePath);

    const std::string& id() const;
    int layerCount() const;
    int middleMinNodes() const;
    int middleMaxNodes() const;
    int combatWeight() const;
    int eventWeight() const;
    const RunMapSpecialNodeConfig& shop() const;
    const RunMapEliteConfig& elites() const;
    const RunMapLayoutConfig& layout() const;

private:
    void validate(const std::filesystem::path& filePath) const;

private:
    std::string id_ = "act1";
    int layerCount_ = 12;
    int middleMinNodes_ = 2;
    int middleMaxNodes_ = 4;
    int combatWeight_ = 70;
    int eventWeight_ = 30;
    RunMapSpecialNodeConfig shop_{1, 2, 8};
    RunMapEliteConfig elites_{1, 2, 3, 8};
    RunMapLayoutConfig layout_;
};
