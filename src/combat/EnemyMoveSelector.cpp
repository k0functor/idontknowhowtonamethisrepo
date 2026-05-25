#include "EnemyMoveSelector.hpp"

#include <algorithm>
#include <stdexcept>

const EnemyActionDefinition& EnemyMoveSelector::selectAction(
    const CombatState& state,
    const EnemyDefinition& enemyDefinition,
    const EntityId,
    Random&
) const {
    if (enemyDefinition.actions.empty()) {
        throw std::runtime_error("Enemy definition has no actions: " + enemyDefinition.id.value);
    }

    const std::size_t index = static_cast<std::size_t>(
        std::max(0, state.turn - 1)
    ) % enemyDefinition.actions.size();

    return enemyDefinition.actions[index];
}

void EnemyMoveSelector::refreshIntents(
    CombatState& state,
    const EnemyDatabase& enemyDatabase,
    Random& random
) const {
    state.enemyIntents.clear();

    for (const CombatEntity& enemy : state.enemies) {
        if (!enemy.isAlive()) {
            continue;
        }

        const EnemyDefinition& definition = enemyDatabase.get(EnemyId(enemy.definitionId));
        const EnemyActionDefinition& action = selectAction(
            state,
            definition,
            enemy.id,
            random
        );

        EnemyIntentState intentState;
        intentState.enemyId = enemy.id;
        intentState.actionId = action.id;
        intentState.intent = makeIntent(action);
        state.enemyIntents.push_back(std::move(intentState));
    }
}

std::optional<EnemyIntentState> EnemyMoveSelector::intentFor(
    const CombatState& state,
    const EntityId enemyId
) const {
    const auto iterator = std::find_if(
        state.enemyIntents.begin(),
        state.enemyIntents.end(),
        [enemyId](const EnemyIntentState& intent) {
            return intent.enemyId == enemyId;
        }
    );

    if (iterator == state.enemyIntents.end()) {
        return std::nullopt;
    }

    return *iterator;
}

EnemyIntent EnemyMoveSelector::makeIntent(const EnemyActionDefinition& action) const {
    EnemyIntent intent;
    intent.type = action.intentType;

    switch (action.intentType) {
        case EnemyIntentType::Attack:
            intent.valueMin = estimateIntentValue(action, EffectType::Damage);
            intent.valueMax = intent.valueMin;
            break;

        case EnemyIntentType::Block:
            intent.valueMin = estimateIntentValue(action, EffectType::Block);
            intent.valueMax = intent.valueMin;
            break;

        case EnemyIntentType::Buff:
        case EnemyIntentType::Debuff:
        case EnemyIntentType::Special:
        case EnemyIntentType::Unknown:
            intent.valueMin = 0;
            intent.valueMax = 0;
            break;
    }

    return intent;
}

int EnemyMoveSelector::estimateIntentValue(
    const EnemyActionDefinition& action,
    const EffectType effectType
) const {
    int total = 0;

    for (const EffectDefinition& effect : action.effects) {
        if (effect.type != effectType) {
            continue;
        }

        total += effect.value.maximumPossibleValue();
    }

    return total;
}
