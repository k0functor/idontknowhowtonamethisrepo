#pragma once

#include "combat/CombatState.hpp"
#include "combat/EnemyMoveSelector.hpp"
#include "combat/EnemyTurnSystem.hpp"
#include "combat/PlayerTurnSystem.hpp"
#include "core/Random.hpp"
#include "data/EnemyDatabase.hpp"

#include <cstddef>

class TurnSystem {
public:
    TurnSystem(
        const EnemyDatabase& enemyDatabase,
        const PlayerTurnSystem& playerTurnSystem,
        const EnemyTurnSystem& enemyTurnSystem,
        const EnemyMoveSelector& enemyMoveSelector,
        std::size_t handSize = 5
    );

    void startCombat(CombatState& state, Random& random) const;
    void endPlayerTurn(CombatState& state, Random& random) const;
    void refreshEnemyIntents(CombatState& state, Random& random) const;

private:
    bool updateCombatResult(CombatState& state) const;
    void startNextPlayerTurn(CombatState& state, Random& random) const;

private:
    const EnemyDatabase& enemyDatabase_;
    const PlayerTurnSystem& playerTurnSystem_;
    const EnemyTurnSystem& enemyTurnSystem_;
    const EnemyMoveSelector& enemyMoveSelector_;
    std::size_t handSize_ = 5;
};
