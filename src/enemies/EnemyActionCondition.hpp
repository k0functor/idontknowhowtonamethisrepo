#pragma once

#include <optional>
#include <string>
#include <vector>

struct EnemyActionCondition {
    int minTurn = 1;
    std::optional<int> maxTurn;

    std::optional<int> selfHpBelowPercent;
    std::optional<int> selfHpAbovePercent;
    std::optional<int> anyPlayerHpBelowPercent;
    std::optional<int> anyPlayerHpAbovePercent;
    std::optional<int> anyOtherEnemyHpBelowPercent;

    int minAliveEnemies = 1;
    std::optional<int> maxAliveEnemies;

    std::vector<std::string> requiredSelfStatuses;
    std::vector<std::string> forbiddenSelfStatuses;
    std::vector<std::string> requiredPlayerStatuses;
    std::vector<std::string> forbiddenPlayerStatuses;
};
