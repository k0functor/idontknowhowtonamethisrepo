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
    const BossPhaseSystem& bossPhaseSystem,
    const StatusSystem& statusSystem,
    const DroneSystem& droneSystem,
    const CombatController& combatController,
    const std::size_t handSize,
    const GameEventBus* eventBus
)
    : enemyDatabase_(enemyDatabase),
      playerTurnSystem_(playerTurnSystem),
      enemyTurnSystem_(enemyTurnSystem),
      enemyMoveSelector_(enemyMoveSelector),
      bossPhaseSystem_(bossPhaseSystem),
      statusSystem_(statusSystem),
      droneSystem_(droneSystem),
      combatController_(combatController),
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

void TurnSystem::endPlayerTurn(CombatState& state, Random& random, const bool skipEnemyActions) const {
    if (state.phase != CombatPhase::PlayerTurn) {
        return;
    }

    bool turnEndedEventEmitted = false;
    if (state.useSequentialPlayerTurns) {
        const std::optional<EntityId> endingPlayer = state.activePlayerId();
        emitTurnEvent(eventBus_, GameEventType::TurnEnded, state);
        turnEndedEventEmitted = true;

        if (endingPlayer.has_value()) {
            statusSystem_.onTurnEndedForEntity(state, *endingPlayer);

            if (updateCombatResult(state)) {
                return;
            }
        }

        if (advanceToNextPlayerSubturn(state)) {
            emitTurnEvent(eventBus_, GameEventType::TurnStarted, state);
            updateCombatResult(state);
            return;
        }
    }

    playerTurnSystem_.endTurn(state, random);
    droneSystem_.processEndOfPlayerTurn(state, random);
    if (!turnEndedEventEmitted) {
        emitTurnEvent(eventBus_, GameEventType::TurnEnded, state);
    }
    if (state.useSequentialPlayerTurns) {
        if (!turnEndedEventEmitted) {
            if (const std::optional<EntityId> endingPlayer = state.activePlayerId()) {
                statusSystem_.onTurnEndedForEntity(state, *endingPlayer);
            }
        }
    } else {
        statusSystem_.onTurnEndedForSide(state, EntityType::Player);
    }

    if (updateCombatResult(state)) {
        return;
    }

    if (!skipEnemyActions) {
        enemyTurnSystem_.executeTurn(state, enemyDatabase_, random);

        if (updateCombatResult(state)) {
            return;
        }
    }

    statusSystem_.onTurnEndedForSide(state, EntityType::Enemy);

    if (updateCombatResult(state)) {
        return;
    }

    startNextPlayerTurn(state, random);
}

void TurnSystem::refreshEnemyIntents(CombatState& state, Random& random) const {
    bossPhaseSystem_.synchronizePhases(state, random);
    bossPhaseSystem_.applyPlayerTurnEffects(state, random);
    enemyMoveSelector_.refreshIntents(state, enemyDatabase_, random);
}

void TurnSystem::refreshEnemyIntentValues(CombatState& state, Random& random) const {
    if (bossPhaseSystem_.synchronizePhases(state, random)) {
        enemyMoveSelector_.refreshIntents(state, enemyDatabase_, random);
        return;
    }
    enemyMoveSelector_.refreshIntentValues(state, enemyDatabase_);
}

bool TurnSystem::updateCombatResult(CombatState& state) const {
    return combatController_.updateAfterAction(state).outcome != CombatOutcome::Ongoing;
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
            state.log.add(
                CombatLogEntryType::ActivePlayerActor,
                {{"actor", state.players[i].definitionId}}
            );
            return true;
        }
    }

    return false;
}
