#include "TurnSystem.hpp"

TurnSystem::TurnSystem(
    const EnemyDatabase& enemyDatabase,
    const PlayerTurnSystem& playerTurnSystem,
    const EnemyTurnSystem& enemyTurnSystem,
    const EnemyMoveSelector& enemyMoveSelector,
    const std::size_t handSize
)
    : enemyDatabase_(enemyDatabase),
      playerTurnSystem_(playerTurnSystem),
      enemyTurnSystem_(enemyTurnSystem),
      enemyMoveSelector_(enemyMoveSelector),
      handSize_(handSize) {}

void TurnSystem::startCombat(CombatState& state, Random& random) const {
    state.turn = 1;
    state.phase = CombatPhase::PlayerTurn;
    state.resources.resetEnergy();

    playerTurnSystem_.startTurn(state, handSize_, random);
    refreshEnemyIntents(state, random);
    updateCombatResult(state);
}

void TurnSystem::endPlayerTurn(CombatState& state, Random& random) const {
    if (state.phase != CombatPhase::PlayerTurn) {
        return;
    }

    playerTurnSystem_.endTurn(state);

    if (updateCombatResult(state)) {
        return;
    }

    enemyTurnSystem_.executeTurn(state, enemyDatabase_, random);

    if (updateCombatResult(state)) {
        return;
    }

    startNextPlayerTurn(state, random);
}

void TurnSystem::refreshEnemyIntents(CombatState& state, Random& random) const {
    enemyMoveSelector_.refreshIntents(state, enemyDatabase_, random);
}

bool TurnSystem::updateCombatResult(CombatState& state) const {
    if (state.aliveEnemyIds().empty()) {
        state.phase = CombatPhase::Won;
        state.enemyIntents.clear();
        state.log.add("Combat won");
        return true;
    }

    if (state.alivePlayerIds().empty()) {
        state.phase = CombatPhase::Lost;
        state.enemyIntents.clear();
        state.log.add("Combat lost");
        return true;
    }

    return false;
}

void TurnSystem::startNextPlayerTurn(CombatState& state, Random& random) const {
    ++state.turn;
    playerTurnSystem_.startTurn(state, handSize_, random);
    refreshEnemyIntents(state, random);
    updateCombatResult(state);
}
