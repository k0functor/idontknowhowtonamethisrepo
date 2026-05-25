#include "TurnSystem.hpp"

TurnSystem::TurnSystem(
    const EnemyDatabase& enemyDatabase,
    const PlayerTurnSystem& playerTurnSystem,
    const EnemyTurnSystem& enemyTurnSystem,
    const EnemyMoveSelector& enemyMoveSelector,
    const StatusSystem& statusSystem,
    const std::size_t handSize,
    const GameEventBus* eventBus
)
    : enemyDatabase_(enemyDatabase),
      playerTurnSystem_(playerTurnSystem),
      enemyTurnSystem_(enemyTurnSystem),
      enemyMoveSelector_(enemyMoveSelector),
      statusSystem_(statusSystem),
      handSize_(handSize),
      eventBus_(eventBus) {}

namespace {
void emitTurnEvent(const GameEventBus* eventBus, GameEventType type, const CombatState& state) {
    if (eventBus == nullptr) {
        return;
    }

    GameEvent event;
    event.type = type;
    event.turn = state.turn;

    if (!state.players.empty()) {
        event.source = state.players.front().id;
    }

    eventBus->emit(event);
}
}

void TurnSystem::startCombat(CombatState& state, Random& random) const {
    state.turn = 1;
    state.phase = CombatPhase::PlayerTurn;
    state.resources.resetEnergy();

    playerTurnSystem_.startTurn(state, handSize_, random);
    refreshEnemyIntents(state, random);
    emitTurnEvent(eventBus_, GameEventType::CombatStarted, state);
    emitTurnEvent(eventBus_, GameEventType::TurnStarted, state);
    updateCombatResult(state);
}

void TurnSystem::endPlayerTurn(CombatState& state, Random& random) const {
    if (state.phase != CombatPhase::PlayerTurn) {
        return;
    }

    playerTurnSystem_.endTurn(state);
    emitTurnEvent(eventBus_, GameEventType::TurnEnded, state);
    statusSystem_.onTurnEndedForSide(state, EntityType::Player);

    if (updateCombatResult(state)) {
        return;
    }

    enemyTurnSystem_.executeTurn(state, enemyDatabase_, random);

    if (updateCombatResult(state)) {
        return;
    }

    statusSystem_.onTurnEndedForSide(state, EntityType::Enemy);

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
    emitTurnEvent(eventBus_, GameEventType::TurnStarted, state);
    updateCombatResult(state);
}
