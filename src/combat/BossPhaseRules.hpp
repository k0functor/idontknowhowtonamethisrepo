#pragma once

#include "combat/CombatState.hpp"
#include "enemies/EnemyDefinition.hpp"

#include <algorithm>
#include <cstdint>
#include <string_view>

namespace BossPhaseRules {
inline const EnemyAiState* aiStateFor(const CombatState& state, const EntityId enemyId) {
    const auto iterator = std::find_if(
        state.enemyAiStates.begin(),
        state.enemyAiStates.end(),
        [enemyId](const EnemyAiState& value) { return value.enemyId == enemyId; }
    );
    return iterator == state.enemyAiStates.end() ? nullptr : &*iterator;
}

inline int phaseIndexForHp(
    const EnemyDefinition& definition,
    const CombatEntity& enemy,
    const int currentPhaseIndex = -1
) {
    if (definition.phases.empty()) {
        return -1;
    }

    const std::int64_t current = static_cast<std::int64_t>(enemy.health.current()) * 100;
    const std::int64_t maximum = std::max(1, enemy.health.maximum());
    int result = std::max(-1, currentPhaseIndex);
    for (std::size_t index = 0; index < definition.phases.size(); ++index) {
        if (current <= maximum * definition.phases[index].activateBelowHpPercent) {
            result = std::max(result, static_cast<int>(index));
        }
    }
    return result;
}

inline const EnemyPhaseDefinition* activePhase(
    const CombatState& state,
    const EnemyDefinition& definition,
    const CombatEntity& enemy
) {
    const EnemyAiState* aiState = aiStateFor(state, enemy.id);
    const int currentIndex = aiState == nullptr ? -1 : aiState->activePhaseIndex;
    const int index = phaseIndexForHp(definition, enemy, currentIndex);
    if (index < 0 || index >= static_cast<int>(definition.phases.size())) {
        return nullptr;
    }
    return &definition.phases[static_cast<std::size_t>(index)];
}

inline bool actionAllowed(
    const CombatState& state,
    const EnemyDefinition& definition,
    const CombatEntity& enemy,
    const std::string_view actionId
) {
    const EnemyPhaseDefinition* phase = activePhase(state, definition, enemy);
    if (phase == nullptr) {
        return true;
    }
    return std::find(phase->actionIds.begin(), phase->actionIds.end(), actionId) != phase->actionIds.end();
}
}
