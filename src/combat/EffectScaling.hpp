#pragma once

#include "effects/EffectDefinition.hpp"
#include "entities/EntityId.hpp"

#include <optional>

class CombatState;

int effectScalingBonus(
    const CombatState& state,
    const EffectDefinition& effect,
    EntityId source,
    std::optional<EntityId> target
);

int scaledEffectAmount(
    const CombatState& state,
    const EffectDefinition& effect,
    EntityId source,
    std::optional<EntityId> target,
    int baseAmount
);
