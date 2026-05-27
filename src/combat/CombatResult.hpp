#pragma once

#include "combat/CombatOutcome.hpp"
#include "run/RunActorState.hpp"

#include <optional>
#include <string>
#include <vector>

struct CombatResult {
    CombatOutcome outcome = CombatOutcome::Ongoing;

    int turnsTaken = 0;
    int playerHpRemaining = 0;
    int playerHpMaximum = 0;

    int enemiesKilled = 0;
    std::vector<std::string> killedEnemyIds;

    std::vector<RunActorState> actorStates;

    std::optional<std::vector<std::string>> remainingConsumableIds;
};
