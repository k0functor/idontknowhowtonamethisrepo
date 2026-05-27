#pragma once

#include "combat/CombatState.hpp"
#include "combat/EnemyIntentState.hpp"
#include "combat/ModifierSystem.hpp"
#include "core/Random.hpp"
#include "data/EnemyDatabase.hpp"
#include "enemies/EnemyActionDefinition.hpp"

#include <optional>
#include <utility>
#include <vector>

class EnemyMoveSelector {
public:
    explicit EnemyMoveSelector(const ModifierSystem& modifierSystem);

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

    void refreshIntentValues(
        CombatState& state,
        const EnemyDatabase& enemyDatabase
    ) const;

    std::optional<EnemyIntentState> intentFor(
        const CombatState& state,
        EntityId enemyId
    ) const;

private:
    EnemyIntent makeIntent(
        const CombatState& state,
        const CombatEntity& enemy,
        const EnemyActionDefinition& action
    ) const;

    int estimateBlockValue(
        const CombatState& state,
        const CombatEntity& enemy,
        const EffectDefinition& effect
    ) const;

    std::pair<int, int> estimateDamageRange(
        const CombatState& state,
        const CombatEntity& enemy,
        const EffectDefinition& effect
    ) const;

    std::vector<EntityId> candidateTargets(
        const CombatState& state,
        const CombatEntity& enemy,
        EffectTarget target
    ) const;

private:
    const ModifierSystem& modifierSystem_;
};
