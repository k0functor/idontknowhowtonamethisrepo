#pragma once

#include "combat/EffectSystem.hpp"
#include "combat/EnemyMoveSelector.hpp"
#include "combat/CombatState.hpp"
#include "core/Random.hpp"
#include "data/EnemyDatabase.hpp"

class EnemyTurnSystem {
public:
    EnemyTurnSystem(
        const EnemyMoveSelector& moveSelector,
        const EffectSystem& effectSystem
    );

    void executeTurn(
        CombatState& state,
        const EnemyDatabase& enemyDatabase,
        Random& random
    ) const;

private:
    const EnemyActionDefinition& actionForEnemy(
        const CombatState& state,
        const EnemyDatabase& enemyDatabase,
        const CombatEntity& enemy,
        Random& random
    ) const;

private:
    const EnemyMoveSelector& moveSelector_;
    const EffectSystem& effectSystem_;
};
