#include "EnemyMoveSelector.hpp"

#include <algorithm>
#include <stdexcept>
#include <utility>

namespace {
CardId noCardId() {
    return CardId{};
}

DiceCorruption noDiceCorruption() {
    return DiceCorruption{};
}

const EnemyActionDefinition& actionById(
    const EnemyDefinition& definition,
    const std::string& actionId
) {
    const auto iterator = std::find_if(
        definition.actions.begin(),
        definition.actions.end(),
        [&actionId](const EnemyActionDefinition& action) {
            return action.id == actionId;
        }
    );

    if (iterator == definition.actions.end()) {
        throw std::runtime_error(
            "Enemy definition '" + definition.id.value + "' has no action '" + actionId + "'"
        );
    }

    return *iterator;
}
}

EnemyMoveSelector::EnemyMoveSelector(const ModifierSystem& modifierSystem)
    : modifierSystem_(modifierSystem) {}

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
        intentState.intent = makeIntent(state, enemy, action);
        state.enemyIntents.push_back(std::move(intentState));
    }
}

void EnemyMoveSelector::refreshIntentValues(
    CombatState& state,
    const EnemyDatabase& enemyDatabase
) const {
    for (EnemyIntentState& intentState : state.enemyIntents) {
        if (!state.hasEntity(intentState.enemyId)) {
            continue;
        }

        const CombatEntity& enemy = state.entity(intentState.enemyId);
        if (!enemy.isAlive() || !state.isEnemy(enemy.id)) {
            continue;
        }

        const EnemyDefinition& definition = enemyDatabase.get(EnemyId(enemy.definitionId));
        const EnemyActionDefinition& action = actionById(definition, intentState.actionId);
        intentState.intent = makeIntent(state, enemy, action);
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

EnemyIntent EnemyMoveSelector::makeIntent(
    const CombatState& state,
    const CombatEntity& enemy,
    const EnemyActionDefinition& action
) const {
    EnemyIntent intent;
    intent.type = action.intentType;

    switch (action.intentType) {
        case EnemyIntentType::Attack: {
            int totalMin = 0;
            int totalMax = 0;
            int damageEffectCount = 0;
            std::pair<int, int> singleDamageRange{0, 0};
            int singleHitCount = 1;

            for (const EffectDefinition& effect : action.effects) {
                if (effect.type != EffectType::Damage) {
                    continue;
                }

                const std::pair<int, int> range = estimateDamageRange(state, enemy, effect);
                totalMin += range.first * effect.repeatCount;
                totalMax += range.second * effect.repeatCount;
                singleDamageRange = range;
                singleHitCount = effect.repeatCount;
                ++damageEffectCount;
            }

            if (damageEffectCount == 1 && singleHitCount > 1) {
                intent.valueMin = singleDamageRange.first;
                intent.valueMax = singleDamageRange.second;
                intent.hitCount = singleHitCount;
            } else {
                intent.valueMin = totalMin;
                intent.valueMax = totalMax;
                intent.hitCount = 1;
            }
            break;
        }

        case EnemyIntentType::Block:
            for (const EffectDefinition& effect : action.effects) {
                if (effect.type == EffectType::Block) {
                    const int value = estimateBlockValue(state, enemy, effect) * effect.repeatCount;
                    intent.valueMin += value;
                    intent.valueMax += value;
                }
            }
            break;

        case EnemyIntentType::Buff:
        case EnemyIntentType::Debuff:
        case EnemyIntentType::Special:
        case EnemyIntentType::Unknown:
            intent.valueMin = 0;
            intent.valueMax = 0;
            intent.hitCount = 1;
            break;
    }

    return intent;
}

int EnemyMoveSelector::estimateBlockValue(
    const CombatState& state,
    const CombatEntity& enemy,
    const EffectDefinition& effect
) const {
    ModifierContext context;
    context.effectType = EffectType::Block;
    context.source = enemy.id;
    context.target = enemy.id;
    context.hasTarget = true;
    context.cardId = noCardId();
    context.diceCorruption = noDiceCorruption();
    context.preview = true;

    const ModifiedValueRange modified = modifierSystem_.modifyRange(
        state,
        effect.value.minimumPossibleValue(),
        effect.value.maximumPossibleValue(),
        context
    );

    return modified.modifiedMax;
}

std::pair<int, int> EnemyMoveSelector::estimateDamageRange(
    const CombatState& state,
    const CombatEntity& enemy,
    const EffectDefinition& effect
) const {
    const std::vector<EntityId> targets = candidateTargets(state, enemy, effect.target);

    if (targets.empty()) {
        ModifierContext context;
        context.effectType = EffectType::Damage;
        context.source = enemy.id;
        context.target = enemy.id;
        context.hasTarget = false;
        context.cardId = noCardId();
        context.diceCorruption = noDiceCorruption();
        context.preview = true;

        const ModifiedValueRange modified = modifierSystem_.modifyRange(
            state,
            effect.value.minimumPossibleValue(),
            effect.value.maximumPossibleValue(),
            context
        );

        return {modified.modifiedMin, modified.modifiedMax};
    }

    int resultMin = 0;
    int resultMax = 0;
    bool initialized = false;

    for (const EntityId target : targets) {
        ModifierContext context;
        context.effectType = EffectType::Damage;
        context.source = enemy.id;
        context.target = target;
        context.hasTarget = true;
        context.cardId = noCardId();
        context.diceCorruption = noDiceCorruption();
        context.preview = true;

        const ModifiedValueRange modified = modifierSystem_.modifyRange(
            state,
            effect.value.minimumPossibleValue(),
            effect.value.maximumPossibleValue(),
            context
        );

        if (!initialized) {
            resultMin = modified.modifiedMin;
            resultMax = modified.modifiedMax;
            initialized = true;
        } else {
            resultMin = std::min(resultMin, modified.modifiedMin);
            resultMax = std::max(resultMax, modified.modifiedMax);
        }
    }

    return {resultMin, resultMax};
}

std::vector<EntityId> EnemyMoveSelector::candidateTargets(
    const CombatState& state,
    const CombatEntity& enemy,
    const EffectTarget target
) const {
    switch (target) {
        case EffectTarget::Self:
            return {enemy.id};

        case EffectTarget::SingleEnemy:
        case EffectTarget::AllEnemies:
        case EffectTarget::RandomEnemy:
            return state.aliveEnemyIds();

        case EffectTarget::Ally:
        case EffectTarget::AllAllies:
        case EffectTarget::RandomAlly:
            return state.alivePlayerIds();
    }

    return {};
}
