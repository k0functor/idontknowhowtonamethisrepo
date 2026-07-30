#include "CardCost.hpp"

#include <algorithm>

namespace {
constexpr const char* freeNextCardStatusId = "free_next_card";
}

namespace CardCost {

bool consumesFreeNextCard(
    const CombatState& state,
    const EntityId source,
    const CardDefinition& definition
) {
    if (definition.energyCost <= 0) {
        return false;
    }

    if (!state.hasEntity(source)) {
        return false;
    }

    return state.entity(source).statuses.has(freeNextCardStatusId);
}

int effectiveEnergyCost(
    const CombatState& state,
    const EntityId source,
    const CardDefinition& definition
) {
    if (consumesFreeNextCard(state, source, definition)) {
        return 0;
    }

    return std::max(0, definition.energyCost + state.cardCostModifier(source));
}

void consumeFreeNextCard(
    CombatState& state,
    const EntityId source,
    const CardDefinition& definition
) {
    if (!consumesFreeNextCard(state, source, definition)) {
        return;
    }

    state.entity(source).statuses.add(freeNextCardStatusId, -1);
}

}
