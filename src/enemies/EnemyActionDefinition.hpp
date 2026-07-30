#pragma once

#include "effects/EffectDefinition.hpp"
#include "enemies/EnemyActionCondition.hpp"
#include "enemies/EnemyIntent.hpp"

#include <string>
#include <vector>

struct EnemyActionDefinition {
    std::string id;
    EnemyIntentType intentType = EnemyIntentType::Unknown;

    // Weighted, state-aware selection metadata. A cooldown of N prevents this
    // action from being selected during the next N enemy intent refreshes.
    int weight = 1;
    int cooldown = 0;
    int maxConsecutiveUses = 0;
    EnemyActionCondition condition;

    std::vector<EffectDefinition> effects;
};
