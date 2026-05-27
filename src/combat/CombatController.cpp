#include "CombatController.hpp"

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
        result.actorStates.push_back(RunActorState{
            player.definitionId,
            player.health.current(),
            player.health.maximum()
        });
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
            state.log.add("Combat won");
        }
        return;
    }

    if (outcome == CombatOutcome::Defeat) {
        if (state.phase != CombatPhase::Lost) {
            state.phase = CombatPhase::Lost;
            state.enemyIntents.clear();
            state.log.add("Combat lost");
        }
    }
}
