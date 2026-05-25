#pragma once

#include "combat/CombatState.hpp"
#include "core/Random.hpp"
#include "data/EnemyDatabase.hpp"
#include "enemies/EnemyActionDefinition.hpp"

#include <optional>

class EnemyMoveSelector {
public:
    const EnemyActionDefinition& selectAction(
        const CombatState& state,
        const EnemyDefinition& enemyDefinition,
        EntityId enemyId,
        Random& random
    ) const;

    void refreshIntents(
        CombatState& state,
        const EnemyDatabase& enemyDatabase,
        Random& random
    ) const;

    std::optional<EnemyIntentState> intentFor(
        const CombatState& state,
        EntityId enemyId
    ) const;

private:
    EnemyIntent makeIntent(const EnemyActionDefinition& action) const;
    int estimateIntentValue(const EnemyActionDefinition& action, EffectType effectType) const;
};
