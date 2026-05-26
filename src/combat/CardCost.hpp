#pragma once

#include "cards/CardDefinition.hpp"
#include "combat/CombatState.hpp"
#include "entities/EntityId.hpp"

namespace CardCost {

int effectiveEnergyCost(
    const CombatState& state,
    EntityId source,
    const CardDefinition& definition
);

bool consumesFreeNextCard(
    const CombatState& state,
    EntityId source,
    const CardDefinition& definition
);

void consumeFreeNextCard(
    CombatState& state,
    EntityId source,
    const CardDefinition& definition
);

}
