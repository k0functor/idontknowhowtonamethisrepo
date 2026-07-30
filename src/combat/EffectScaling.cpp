#include "combat/EffectScaling.hpp"

#include "combat/CombatState.hpp"

#include <algorithm>

namespace {
int scalingStatusStacks(
    const CombatState& state,
    const EffectScalingDefinition& scaling,
    const EntityId source,
    const std::optional<EntityId> target
) {
    if (!scaling.statusId.has_value()) {
        return 0;
    }

    const std::optional<EntityId> owner = scaling.statusOwner == EffectScalingStatusOwner::Source
        ? std::optional<EntityId>(source)
        : target;

    if (!owner.has_value() || !state.hasEntity(*owner)) {
        return 0;
    }

    return state.entity(*owner).statuses.stacks(*scaling.statusId);
}
}

int effectScalingBonus(
    const CombatState& state,
    const EffectDefinition& effect,
    const EntityId source,
    const std::optional<EntityId> target
) {
    const EffectScalingDefinition& scaling = effect.scaling;
    int bonus = 0;

    const int statusStacks = scalingStatusStacks(state, scaling, source, target);
    if (statusStacks > 0) {
        bonus += scaling.bonusIfStatusPresent;
        bonus += scaling.bonusPerStatusStack * statusStacks;
    }

    bonus += scaling.bonusPerCardInHand * static_cast<int>(state.hand.size());
    bonus += scaling.bonusPerCardInDiscard * static_cast<int>(state.deck.discardPile.size());

    if (scaling.maximumBonus >= 0) {
        bonus = std::min(bonus, scaling.maximumBonus);
    }

    return bonus;
}

int scaledEffectAmount(
    const CombatState& state,
    const EffectDefinition& effect,
    const EntityId source,
    const std::optional<EntityId> target,
    const int baseAmount
) {
    return std::max(0, baseAmount + effectScalingBonus(state, effect, source, target));
}
