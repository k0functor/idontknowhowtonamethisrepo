#include "ModifierSystem.hpp"

#include "combat/CombatState.hpp"

#include <algorithm>
#include <cmath>

namespace {
constexpr const char* strengthStatusId = "strength";
constexpr const char* weakStatusId = "weak";
constexpr const char* vulnerableStatusId = "vulnerable";
constexpr const char* dexterityStatusId = "dexterity";
constexpr const char* stanceFlameStatusId = "stance_flame";
constexpr const char* stanceAshStatusId = "stance_ash";
constexpr const char* stanceSmokeStatusId = "stance_smoke";
}

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

    collectBuiltInStatusModifiers(state, context, result);

    for (const IModifierProvider* provider : providers_) {
        provider->collectModifiers(state, context, result);
    }

    return result;
}

void ModifierSystem::collectBuiltInStatusModifiers(
    const CombatState& state,
    const ModifierContext& context,
    std::vector<ValueModifier>& output
) const {
    if (!state.hasEntity(context.source)) {
        return;
    }

    const CombatEntity& source = state.entity(context.source);

    const CombatEntity* target = nullptr;
    if (context.hasTarget) {
        if (!state.hasEntity(context.target)) {
            return;
        }

        target = &state.entity(context.target);
    }

    if (context.effectType == EffectType::Damage) {
        const int strength = source.statuses.stacks(strengthStatusId);
        if (strength > 0) {
            output.push_back({
                strengthStatusId,
                "Strength adds outgoing damage",
                ModifierOperation::Add,
                strength,
                1.0,
                100
            });
        }

        if (source.statuses.has(stanceFlameStatusId)) {
            output.push_back({
                stanceFlameStatusId,
                "Flame stance increases outgoing damage by 25%",
                ModifierOperation::Multiply,
                0,
                1.25,
                150
            });
        }

        if (source.statuses.has(stanceAshStatusId)) {
            output.push_back({
                stanceAshStatusId,
                "Ash stance reduces outgoing damage by 15%",
                ModifierOperation::Multiply,
                0,
                0.85,
                150
            });
        }

        if (source.statuses.has(weakStatusId)) {
            output.push_back({
                weakStatusId,
                "Weak reduces outgoing damage",
                ModifierOperation::Multiply,
                0,
                0.75,
                200
            });
        }

        if (target != nullptr && target->statuses.has(stanceFlameStatusId)) {
            output.push_back({
                stanceFlameStatusId,
                "Flame stance increases incoming damage by 25%",
                ModifierOperation::Multiply,
                0,
                1.25,
                250
            });
        }

        if (target != nullptr && target->statuses.has(stanceSmokeStatusId)) {
            output.push_back({
                stanceSmokeStatusId,
                "Smoke stance reduces incoming damage by 25%",
                ModifierOperation::Multiply,
                0,
                0.75,
                250
            });
        }

        if (target != nullptr && target->statuses.has(vulnerableStatusId)) {
            output.push_back({
                vulnerableStatusId,
                "Vulnerable increases incoming damage",
                ModifierOperation::Multiply,
                0,
                1.5,
                300
            });
        }
    }

    if (context.effectType == EffectType::Block) {
        const int dexterity = source.statuses.stacks(dexterityStatusId);
        if (source.statuses.has(stanceAshStatusId)) {
            output.push_back({
                stanceAshStatusId,
                "Ash stance adds block",
                ModifierOperation::Add,
                2,
                1.0,
                90
            });
        }

        if (dexterity > 0) {
            output.push_back({
                dexterityStatusId,
                "Dexterity adds block",
                ModifierOperation::Add,
                dexterity,
                1.0,
                100
            });
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
