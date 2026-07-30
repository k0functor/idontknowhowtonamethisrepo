#pragma once

#include "combat/CombatState.hpp"
#include "combat/EffectSystem.hpp"
#include "core/Random.hpp"
#include "data/EnemyDatabase.hpp"

class BossPhaseSystem {
public:
    BossPhaseSystem(const EnemyDatabase& enemyDatabase, const EffectSystem& effectSystem);

    bool synchronizePhases(CombatState& state, Random& random) const;
    void applyPlayerTurnEffects(CombatState& state, Random& random) const;

private:
    const EnemyDatabase& enemyDatabase_;
    const EffectSystem& effectSystem_;
};
