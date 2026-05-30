#include "CombatController.hpp"

#include "run/StressRules.hpp"

#include <algorithm>
#include <utility>

CombatOutcome CombatController::checkOutcome(const CombatState& state) const {
    if (state.aliveEnemyIds().empty()) {
        return CombatOutcome::Victory;
    }

    if (state.alivePlayerIds().empty()) {
        return CombatOutcome::Defeat;
    }

    return CombatOutcome::Ongoing;
}

CombatResult CombatController::buildResult(const CombatState& state) const {
    CombatResult result;
    result.outcome = checkOutcome(state);
    result.turnsTaken = state.turn;

    result.actorStates.reserve(state.players.size());
    for (const CombatEntity& player : state.players) {
        result.playerHpRemaining += player.health.current();
        result.playerHpMaximum += player.health.maximum();
        RunActorState actorState;
        actorState.definitionId = player.definitionId;
        actorState.currentHp = player.health.current();
        actorState.maxHp = player.health.maximum();
        actorState.stress = std::clamp(player.stress, 0, std::max(StressRules::MaximumStress, player.maxStress));
        actorState.maxStress = std::max(StressRules::MaximumStress, player.maxStress);
        actorState.resolveCheckTriggered = player.resolveCheckTriggered;
        actorState.traitIds = player.traitIds;
        StressRules::normalize(actorState);
        result.actorStates.push_back(std::move(actorState));
    }

    for (const CombatEntity& enemy : state.enemies) {
        if (!enemy.isAlive()) {
            ++result.enemiesKilled;
            result.killedEnemyIds.push_back(enemy.definitionId);
        }
    }

    return result;
}

CombatResult CombatController::updateAfterAction(CombatState& state) const {
    const CombatOutcome outcome = checkOutcome(state);
    applyOutcomeToState(state, outcome);
    return buildResult(state);
}

void CombatController::applyOutcomeToState(
    CombatState& state,
    const CombatOutcome outcome
) const {
    if (outcome == CombatOutcome::Ongoing) {
        return;
    }

    if (outcome == CombatOutcome::Victory) {
        if (state.phase != CombatPhase::Won) {
            state.phase = CombatPhase::Won;
            state.enemyIntents.clear();
            state.log.add(CombatLogEntryType::CombatWon);
        }
        return;
    }

    if (outcome == CombatOutcome::Defeat) {
        if (state.phase != CombatPhase::Lost) {
            state.phase = CombatPhase::Lost;
            state.enemyIntents.clear();
            state.log.add(CombatLogEntryType::CombatLost);
        }
    }
}
