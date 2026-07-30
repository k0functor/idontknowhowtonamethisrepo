#pragma once

#include "effects/EffectDefinition.hpp"
#include "localization/TextId.hpp"

#include <string>
#include <vector>

struct EnemyPhaseDefinition {
    std::string id;
    TextId nameTextId;

    // The phase becomes active once current HP is at or below this percentage.
    // Phases are declared from 100 downwards and never regress after activation.
    int activateBelowHpPercent = 100;

    std::vector<std::string> actionIds;
    std::vector<EffectDefinition> onEnterEffects;
    std::vector<EffectDefinition> playerTurnEffects;
    std::vector<std::string> summonEnemyIds;

    // The combat UI currently supports at most three living enemies cleanly.
    int maximumAliveEnemies = 3;
};
