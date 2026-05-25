#pragma once

#include "effects/EffectDefinition.hpp"
#include "enemies/EnemyIntent.hpp"

#include <string>
#include <vector>

struct EnemyActionDefinition {
    std::string id;
    EnemyIntentType intentType = EnemyIntentType::Unknown;
    std::vector<EffectDefinition> effects;
};
