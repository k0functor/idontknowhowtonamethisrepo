#pragma once

#include "entities/EntityId.hpp"

#include <string>
#include <vector>

struct EnemyActionCooldownState {
    std::string actionId;
    int availableOnTurn = 1;
};

struct EnemyAiState {
    EntityId enemyId;
    std::string lastActionId;
    int consecutiveUses = 0;
    std::vector<EnemyActionCooldownState> cooldowns;

    int activePhaseIndex = -1;
    std::string activePhaseId;
    int lastArenaEffectTurn = 0;
};
