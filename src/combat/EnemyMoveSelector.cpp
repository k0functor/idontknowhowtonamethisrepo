#include "EnemyMoveSelector.hpp"
#include "combat/BossPhaseRules.hpp"

#include <algorithm>
#include <cstdint>
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

std::pair<int, int> rawValueRange(const EffectDefinition& effect) {
    return {effect.value.minimumPossibleValue(), effect.value.maximumPossibleValue()};
}

bool hpAtOrBelowPercent(const CombatEntity& entity, const int percent) {
    return static_cast<std::int64_t>(entity.health.current()) * 100 <=
        static_cast<std::int64_t>(entity.health.maximum()) * percent;
}

bool hpAtOrAbovePercent(const CombatEntity& entity, const int percent) {
    return static_cast<std::int64_t>(entity.health.current()) * 100 >=
        static_cast<std::int64_t>(entity.health.maximum()) * percent;
}

const EnemyAiState* aiStateFor(const CombatState& state, const EntityId enemyId) {
    const auto iterator = std::find_if(
        state.enemyAiStates.begin(),
        state.enemyAiStates.end(),
        [enemyId](const EnemyAiState& aiState) {
            return aiState.enemyId == enemyId;
        }
    );
    return iterator == state.enemyAiStates.end() ? nullptr : &*iterator;
}

EnemyAiState& mutableAiStateFor(CombatState& state, const EntityId enemyId) {
    const auto iterator = std::find_if(
        state.enemyAiStates.begin(),
        state.enemyAiStates.end(),
        [enemyId](const EnemyAiState& aiState) {
            return aiState.enemyId == enemyId;
        }
    );
    if (iterator != state.enemyAiStates.end()) {
        return *iterator;
    }

    EnemyAiState aiState;
    aiState.enemyId = enemyId;
    state.enemyAiStates.push_back(std::move(aiState));
    return state.enemyAiStates.back();
}

bool hasAllStatuses(
    const CombatEntity& entity,
    const std::vector<std::string>& statusIds
) {
    return std::all_of(
        statusIds.begin(),
        statusIds.end(),
        [&entity](const std::string& statusId) {
            return entity.statuses.has(statusId);
        }
    );
}

bool hasAnyStatus(
    const CombatEntity& entity,
    const std::vector<std::string>& statusIds
) {
    return std::any_of(
        statusIds.begin(),
        statusIds.end(),
        [&entity](const std::string& statusId) {
            return entity.statuses.has(statusId);
        }
    );
}

bool playersSatisfyRequiredStatuses(
    const CombatState& state,
    const std::vector<std::string>& statusIds
) {
    return std::all_of(
        statusIds.begin(),
        statusIds.end(),
        [&state](const std::string& statusId) {
            return std::any_of(
                state.players.begin(),
                state.players.end(),
                [&statusId](const CombatEntity& player) {
                    return player.isAlive() && player.statuses.has(statusId);
                }
            );
        }
    );
}

bool playersAvoidForbiddenStatuses(
    const CombatState& state,
    const std::vector<std::string>& statusIds
) {
    return std::none_of(
        state.players.begin(),
        state.players.end(),
        [&statusIds](const CombatEntity& player) {
            return player.isAlive() && hasAnyStatus(player, statusIds);
        }
    );
}

bool conditionsPass(
    const CombatState& state,
    const CombatEntity& enemy,
    const EnemyActionCondition& condition
) {
    if (state.turn < condition.minTurn) {
        return false;
    }
    if (condition.maxTurn.has_value() && state.turn > *condition.maxTurn) {
        return false;
    }

    const int aliveEnemies = static_cast<int>(state.aliveEnemyCount());
    if (aliveEnemies < condition.minAliveEnemies) {
        return false;
    }
    if (condition.maxAliveEnemies.has_value() && aliveEnemies > *condition.maxAliveEnemies) {
        return false;
    }

    if (condition.selfHpBelowPercent.has_value() &&
        !hpAtOrBelowPercent(enemy, *condition.selfHpBelowPercent)) {
        return false;
    }
    if (condition.selfHpAbovePercent.has_value() &&
        !hpAtOrAbovePercent(enemy, *condition.selfHpAbovePercent)) {
        return false;
    }

    if (condition.anyPlayerHpBelowPercent.has_value()) {
        const bool found = std::any_of(
            state.players.begin(),
            state.players.end(),
            [&condition](const CombatEntity& player) {
                return player.isAlive() &&
                    hpAtOrBelowPercent(player, *condition.anyPlayerHpBelowPercent);
            }
        );
        if (!found) {
            return false;
        }
    }

    if (condition.anyPlayerHpAbovePercent.has_value()) {
        const bool found = std::any_of(
            state.players.begin(),
            state.players.end(),
            [&condition](const CombatEntity& player) {
                return player.isAlive() &&
                    hpAtOrAbovePercent(player, *condition.anyPlayerHpAbovePercent);
            }
        );
        if (!found) {
            return false;
        }
    }

    if (condition.anyOtherEnemyHpBelowPercent.has_value()) {
        const bool found = std::any_of(
            state.enemies.begin(),
            state.enemies.end(),
            [&condition, &enemy](const CombatEntity& ally) {
                return ally.isAlive() && ally.id != enemy.id &&
                    hpAtOrBelowPercent(ally, *condition.anyOtherEnemyHpBelowPercent);
            }
        );
        if (!found) {
            return false;
        }
    }

    if (!hasAllStatuses(enemy, condition.requiredSelfStatuses) ||
        hasAnyStatus(enemy, condition.forbiddenSelfStatuses)) {
        return false;
    }
    if (!playersSatisfyRequiredStatuses(state, condition.requiredPlayerStatuses) ||
        !playersAvoidForbiddenStatuses(state, condition.forbiddenPlayerStatuses)) {
        return false;
    }

    return true;
}

bool historyConstraintsPass(
    const CombatState& state,
    const EntityId enemyId,
    const EnemyActionDefinition& action
) {
    const EnemyAiState* aiState = aiStateFor(state, enemyId);
    if (aiState == nullptr) {
        return true;
    }

    const auto cooldown = std::find_if(
        aiState->cooldowns.begin(),
        aiState->cooldowns.end(),
        [&action](const EnemyActionCooldownState& entry) {
            return entry.actionId == action.id;
        }
    );
    if (cooldown != aiState->cooldowns.end() && state.turn < cooldown->availableOnTurn) {
        return false;
    }

    if (action.maxConsecutiveUses > 0 &&
        aiState->lastActionId == action.id &&
        aiState->consecutiveUses >= action.maxConsecutiveUses) {
        return false;
    }

    return true;
}

const EnemyActionDefinition& weightedChoice(
    const std::vector<const EnemyActionDefinition*>& candidates,
    Random& random
) {
    int totalWeight = 0;
    for (const EnemyActionDefinition* action : candidates) {
        totalWeight += action->weight;
    }

    int roll = random.rangeInclusive(1, totalWeight);
    for (const EnemyActionDefinition* action : candidates) {
        roll -= action->weight;
        if (roll <= 0) {
            return *action;
        }
    }

    return *candidates.back();
}

void rememberSelectedAction(
    CombatState& state,
    const EntityId enemyId,
    const EnemyActionDefinition& action
) {
    EnemyAiState& aiState = mutableAiStateFor(state, enemyId);
    if (aiState.lastActionId == action.id) {
        ++aiState.consecutiveUses;
    } else {
        aiState.lastActionId = action.id;
        aiState.consecutiveUses = 1;
    }

    const int availableOnTurn = state.turn + action.cooldown + 1;
    const auto cooldown = std::find_if(
        aiState.cooldowns.begin(),
        aiState.cooldowns.end(),
        [&action](const EnemyActionCooldownState& entry) {
            return entry.actionId == action.id;
        }
    );
    if (cooldown == aiState.cooldowns.end()) {
        aiState.cooldowns.push_back({action.id, availableOnTurn});
    } else {
        cooldown->availableOnTurn = availableOnTurn;
    }
}
} // namespace

EnemyMoveSelector::EnemyMoveSelector(const ModifierSystem& modifierSystem)
    : modifierSystem_(modifierSystem) {}

const EnemyActionDefinition& EnemyMoveSelector::selectAction(
    const CombatState& state,
    const EnemyDefinition& enemyDefinition,
    const EntityId enemyId,
    Random& random
) const {
    if (enemyDefinition.actions.empty()) {
        throw std::runtime_error("Enemy definition has no actions: " + enemyDefinition.id.value);
    }
    if (!state.hasEntity(enemyId) || !state.isEnemy(enemyId)) {
        throw std::runtime_error("Enemy action selection requires a living enemy entity");
    }

    const CombatEntity& enemy = state.entity(enemyId);
    std::vector<const EnemyActionDefinition*> eligible;
    std::vector<const EnemyActionDefinition*> conditionEligible;
    eligible.reserve(enemyDefinition.actions.size());
    conditionEligible.reserve(enemyDefinition.actions.size());

    for (const EnemyActionDefinition& action : enemyDefinition.actions) {
        if (!BossPhaseRules::actionAllowed(state, enemyDefinition, enemy, action.id)) {
            continue;
        }
        if (!conditionsPass(state, enemy, action.condition)) {
            continue;
        }

        conditionEligible.push_back(&action);
        if (historyConstraintsPass(state, enemyId, action)) {
            eligible.push_back(&action);
        }
    }

    // A malformed or extremely restrictive data set must not soft-lock combat.
    // Prefer actions whose state conditions still make sense, then fall back to
    // the full move set only as a final safety valve.
    if (!eligible.empty()) {
        return weightedChoice(eligible, random);
    }
    if (!conditionEligible.empty()) {
        return weightedChoice(conditionEligible, random);
    }

    std::vector<const EnemyActionDefinition*> fallback;
    fallback.reserve(enemyDefinition.actions.size());
    for (const EnemyActionDefinition& action : enemyDefinition.actions) {
        if (BossPhaseRules::actionAllowed(state, enemyDefinition, enemy, action.id)) {
            fallback.push_back(&action);
        }
    }
    if (fallback.empty()) {
        for (const EnemyActionDefinition& action : enemyDefinition.actions) {
            fallback.push_back(&action);
        }
    }
    return weightedChoice(fallback, random);
}

void EnemyMoveSelector::refreshIntents(
    CombatState& state,
    const EnemyDatabase& enemyDatabase,
    Random& random
) const {
    state.enemyIntents.clear();
    std::erase_if(
        state.enemyAiStates,
        [&state](const EnemyAiState& aiState) {
            return !state.hasEntity(aiState.enemyId) || !state.entity(aiState.enemyId).isAlive();
        }
    );

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
        rememberSelectedAction(state, enemy.id, action);

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
    intent.effectSummaries.reserve(action.effects.size());

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

    for (const EffectDefinition& effect : action.effects) {
        EnemyIntentEffectSummary summary;
        summary.type = effect.type;
        summary.target = effect.target;
        summary.repeatCount = std::max(1, effect.repeatCount);
        if (effect.statusId.has_value()) {
            summary.statusId = *effect.statusId;
        }

        switch (effect.type) {
            case EffectType::Damage: {
                const std::pair<int, int> range = estimateDamageRange(state, enemy, effect);
                summary.valueMin = range.first;
                summary.valueMax = range.second;
                break;
            }

            case EffectType::Block:
                summary.valueMin = estimateBlockValue(state, enemy, effect);
                summary.valueMax = summary.valueMin;
                break;

            case EffectType::Heal:
            case EffectType::ApplyStatus:
            case EffectType::DrawCards:
            case EffectType::DiscardCards:
            case EffectType::RecoverCards:
            case EffectType::GainEnergy:
            case EffectType::GainStress:
            case EffectType::SpendStressDamage:
            case EffectType::SpendStressBlock:
            case EffectType::SpendStressEnergy:
            case EffectType::SpendStressDraw:
            case EffectType::LoseEnergy:
            case EffectType::LoseStress:
            case EffectType::LoseHp:
            case EffectType::EnterStance:
            case EffectType::SummonDrone:
            case EffectType::UseDrone:
            case EffectType::PrimeStressBreakdown: {
                const std::pair<int, int> range = rawValueRange(effect);
                summary.valueMin = range.first;
                summary.valueMax = range.second;
                break;
            }
        }

        intent.effectSummaries.push_back(std::move(summary));
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
    context.usesActorStats = true;
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
        context.usesActorStats = true;
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
        context.usesActorStats = true;
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
