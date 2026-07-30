#pragma once

#include "combat/CombatController.hpp"
#include "combat/CombatState.hpp"
#include "combat/BossPhaseSystem.hpp"
#include "combat/EnemyMoveSelector.hpp"
#include "combat/EnemyTurnSystem.hpp"
#include "combat/PlayerTurnSystem.hpp"
#include "core/Random.hpp"
#include "data/EnemyDatabase.hpp"
#include "statuses/StatusSystem.hpp"
#include "drones/DroneSystem.hpp"
#include "game/GameEventBus.hpp"

#include <cstddef>

class TurnSystem {
public:
    TurnSystem(
        const EnemyDatabase& enemyDatabase,
        const PlayerTurnSystem& playerTurnSystem,
        const EnemyTurnSystem& enemyTurnSystem,
        const EnemyMoveSelector& enemyMoveSelector,
        const BossPhaseSystem& bossPhaseSystem,
        const StatusSystem& statusSystem,
        const DroneSystem& droneSystem,
        const CombatController& combatController,
        std::size_t handSize = 5,
        const GameEventBus* eventBus = nullptr
    );

    void startCombat(CombatState& state, Random& random) const;
    void endPlayerTurn(CombatState& state, Random& random, bool skipEnemyActions = false) const;
    void refreshEnemyIntents(CombatState& state, Random& random) const;
    void refreshEnemyIntentValues(CombatState& state, Random& random) const;

private:
    bool updateCombatResult(CombatState& state) const;
    void startNextPlayerTurn(CombatState& state, Random& random) const;
    void setActivePlayerToFirstAlive(CombatState& state) const;
    bool advanceToNextPlayerSubturn(CombatState& state) const;

private:
    const EnemyDatabase& enemyDatabase_;
    const PlayerTurnSystem& playerTurnSystem_;
    const EnemyTurnSystem& enemyTurnSystem_;
    const EnemyMoveSelector& enemyMoveSelector_;
    const BossPhaseSystem& bossPhaseSystem_;
    const StatusSystem& statusSystem_;
    const DroneSystem& droneSystem_;
    const CombatController& combatController_;
    std::size_t handSize_ = 5;
    const GameEventBus* eventBus_ = nullptr;
};
