#include "TurnSystem.hpp"

#include <algorithm>
#include <optional>
#include <string>
#include <vector>

TurnSystem::TurnSystem(
    const EnemyDatabase& enemyDatabase,
    const PlayerTurnSystem& playerTurnSystem,
    const EnemyTurnSystem& enemyTurnSystem,
    const EnemyMoveSelector& enemyMoveSelector,
    const StatusSystem& statusSystem,
    const DroneSystem& droneSystem,
    const std::size_t handSize,
    const GameEventBus* eventBus
)
    : enemyDatabase_(enemyDatabase),
      playerTurnSystem_(playerTurnSystem),
      enemyTurnSystem_(enemyTurnSystem),
      enemyMoveSelector_(enemyMoveSelector),
      statusSystem_(statusSystem),
      droneSystem_(droneSystem),
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

    if (const std::optional<EntityId> activePlayer = state.activePlayerId()) {
        event.source = *activePlayer;
    } else if (!state.players.empty()) {
        event.source = state.players.front().id;
    }

    eventBus->emit(event);
}
}

void TurnSystem::startCombat(CombatState& state, Random& random) const {
    state.turn = 1;
    state.phase = CombatPhase::PlayerTurn;
    setActivePlayerToFirstAlive(state);
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

    bool turnEndedEventEmitted = false;
    if (state.useSequentialPlayerTurns) {
        emitTurnEvent(eventBus_, GameEventType::TurnEnded, state);
        turnEndedEventEmitted = true;
        if (advanceToNextPlayerSubturn(state)) {
            emitTurnEvent(eventBus_, GameEventType::TurnStarted, state);
            updateCombatResult(state);
            return;
        }
    }

    playerTurnSystem_.endTurn(state);
    droneSystem_.processEndOfPlayerTurn(state, random);
    if (!turnEndedEventEmitted) {
        emitTurnEvent(eventBus_, GameEventType::TurnEnded, state);
    }
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

void TurnSystem::refreshEnemyIntentValues(CombatState& state) const {
    enemyMoveSelector_.refreshIntentValues(state, enemyDatabase_);
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
    setActivePlayerToFirstAlive(state);
    playerTurnSystem_.startTurn(state, handSize_, random);
    refreshEnemyIntents(state, random);
    emitTurnEvent(eventBus_, GameEventType::TurnStarted, state);
    updateCombatResult(state);
}

void TurnSystem::setActivePlayerToFirstAlive(CombatState& state) const {
    state.activePlayerIndex = 0;

    for (std::size_t i = 0; i < state.players.size(); ++i) {
        if (state.players[i].isAlive()) {
            state.activePlayerIndex = i;
            return;
        }
    }
}

bool TurnSystem::advanceToNextPlayerSubturn(CombatState& state) const {
    if (state.players.empty()) {
        return false;
    }

    const std::size_t start = std::min(state.activePlayerIndex + 1, state.players.size());
    for (std::size_t i = start; i < state.players.size(); ++i) {
        if (state.players[i].isAlive()) {
            state.activePlayerIndex = i;
            state.log.add("Active player actor: " + state.players[i].definitionId);
            return true;
        }
    }

    return false;
}
