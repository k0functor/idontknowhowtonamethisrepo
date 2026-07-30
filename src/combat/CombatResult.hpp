#pragma once

#include "combat/CombatOutcome.hpp"
#include "combat/CombatTelemetry.hpp"
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
    CombatTelemetry telemetry;
    std::vector<std::string> killedEnemyIds;
    std::vector<std::string> encounteredEnemyIds;
    std::vector<std::string> statusIdsSeen;
    std::vector<std::string> playedCardIds;

    std::vector<RunActorState> actorStates;

    std::optional<std::vector<std::string>> remainingConsumableIds;
};
