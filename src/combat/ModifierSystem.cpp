#include "ModifierSystem.hpp"

#include "combat/CombatState.hpp"
#include "localization/LocalizationManager.hpp"
#include "run/StressPsychopathRules.hpp"

#include <algorithm>
#include <cmath>

ModifierSystem::ModifierSystem(
    const LocalizationManager& localization,
    const StatusDatabase& statusDatabase
)
    : localization_(localization),
      statusDatabase_(statusDatabase) {}

void ModifierSystem::addProvider(const IModifierProvider& provider) {
    providers_.push_back(&provider);
}

void ModifierSystem::clearProviders() {
    providers_.clear();
}

ModifiedValue ModifierSystem::modifyValue(
    const CombatState& state,
    const int baseValue,
    const ModifierContext& context
) const {
    std::vector<ValueModifier> modifiers = collectModifiers(state, context);

    std::sort(modifiers.begin(), modifiers.end(), [](const ValueModifier& lhs, const ValueModifier& rhs) {
        return lhs.priority < rhs.priority;
    });

    ModifiedValue result;
    result.base = baseValue;
    result.modified = baseValue;

    for (const ValueModifier& modifier : modifiers) {
        const int before = result.modified;
        result.modified = applyModifier(result.modified, modifier);

        result.breakdown.push_back({
            modifier.sourceId,
            modifier.description,
            before,
            result.modified
        });
    }

    if (result.modified < 0) {
        result.modified = 0;
    }

    return result;
}

ModifiedValueRange ModifierSystem::modifyRange(
    const CombatState& state,
    const int baseMin,
    const int baseMax,
    const ModifierContext& context
) const {
    const ModifiedValue minValue = modifyValue(state, baseMin, context);
    const ModifiedValue maxValue = modifyValue(state, baseMax, context);

    ModifiedValueRange result;
    result.baseMin = baseMin;
    result.baseMax = baseMax;
    result.modifiedMin = minValue.modified;
    result.modifiedMax = maxValue.modified;

    for (const ModifierBreakdownEntry& entry : maxValue.breakdown) {
        result.modifierLabels.push_back(entry.sourceId);
    }

    return result;
}

std::vector<ValueModifier> ModifierSystem::collectModifiers(
    const CombatState& state,
    const ModifierContext& context
) const {
    std::vector<ValueModifier> result;

    collectStatusModifiers(state, context, result);

    if (context.effectType == EffectType::Damage &&
        state.hasEntity(context.source) &&
        state.isEnemy(context.source) &&
        std::abs(state.enemyDamageMultiplier - 1.f) > 0.0001f) {
        result.push_back({
            "difficulty_enemy_damage",
            localization_.get(TextId("modifier.difficulty.enemy_damage_multiplier")),
            ModifierOperation::Multiply,
            0,
            static_cast<double>(state.enemyDamageMultiplier),
            25
        });
    }

    for (const IModifierProvider* provider : providers_) {
        provider->collectModifiers(state, context, result);
    }

    return result;
}

void ModifierSystem::collectStatusModifiers(
    const CombatState& state,
    const ModifierContext& context,
    std::vector<ValueModifier>& output
) const {
    if (!state.hasEntity(context.source)) {
        return;
    }

    collectModifiersFromEntity(
        state,
        context,
        context.source,
        StatusModifierEntity::Source,
        output
    );

    if (context.hasTarget && state.hasEntity(context.target)) {
        collectModifiersFromEntity(
            state,
            context,
            context.target,
            StatusModifierEntity::Target,
            output
        );
    }

    const CombatEntity& source = state.entity(context.source);
    if (context.effectType == EffectType::Damage &&
        context.usesActorStats &&
        StressPsychopathRules::appliesTo(source.definitionId)) {
        const int stressDamageBonus = StressPsychopathRules::damageBonusForStress(source.stress);
        if (stressDamageBonus > 0) {
            output.push_back({
                "lost_psychopath_stress",
                localization_.get(TextId("modifier.mechanic.lost_psychopath.stress_damage_add")),
                ModifierOperation::Add,
                stressDamageBonus,
                1.0,
                125
            });
        }
    }
}

void ModifierSystem::collectModifiersFromEntity(
    const CombatState& state,
    const ModifierContext& context,
    const EntityId entityId,
    const StatusModifierEntity modifierEntity,
    std::vector<ValueModifier>& output
) const {
    const CombatEntity& entity = state.entity(entityId);

    for (const auto& [statusId, stacks] : entity.statuses.all()) {
        if (stacks <= 0 || !statusDatabase_.contains(StatusId(statusId))) {
            continue;
        }

        const StatusDefinition& definition = statusDatabase_.get(StatusId(statusId));
        for (const StatusModifierDefinition& modifier : definition.modifiers) {
            if (modifier.entity != modifierEntity || modifier.effectType != context.effectType) {
                continue;
            }
            if (modifier.requiresActorStats && !context.usesActorStats) {
                continue;
            }

            ValueModifier valueModifier;
            valueModifier.sourceId = statusId;
            valueModifier.description = localization_.get(modifier.descriptionTextId);
            valueModifier.priority = modifier.priority;

            switch (modifier.operation) {
                case StatusModifierOperation::AddPerStack:
                    valueModifier.operation = ModifierOperation::Add;
                    valueModifier.addAmount = modifier.addAmount * stacks;
                    break;

                case StatusModifierOperation::AddFixed:
                    valueModifier.operation = ModifierOperation::Add;
                    valueModifier.addAmount = modifier.addAmount;
                    break;

                case StatusModifierOperation::MultiplyPerStack:
                    valueModifier.operation = ModifierOperation::Multiply;
                    valueModifier.multiplier = 1.0 + modifier.multiplier * static_cast<double>(stacks);
                    break;

                case StatusModifierOperation::MultiplyFixed:
                    valueModifier.operation = ModifierOperation::Multiply;
                    valueModifier.multiplier = modifier.multiplier;
                    break;
            }

            output.push_back(std::move(valueModifier));
        }
    }
}

int ModifierSystem::applyModifier(const int value, const ValueModifier& modifier) {
    switch (modifier.operation) {
        case ModifierOperation::Add:
            return value + modifier.addAmount;
        case ModifierOperation::Multiply:
            return static_cast<int>(std::floor(static_cast<double>(value) * modifier.multiplier));
    }

    return value;
}
