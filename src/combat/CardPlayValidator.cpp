#include "CardPlayValidator.hpp"

#include "cards/CardKeyword.hpp"

#include <algorithm>

CardPlayValidationResult CardPlayValidator::validate(
    const CombatState& state,
    const CardDefinition& definition,
    const CardInstance& instance
) const {
    if (state.players.empty()) {
        return {false, "No player actor"};
    }

    return validate(state, definition, instance, state.players.front().id);
}

CardPlayValidationResult CardPlayValidator::validate(
    const CombatState& state,
    const CardDefinition& definition,
    const CardInstance& instance,
    const EntityId source
) const {
    if (state.phase != CombatPhase::PlayerTurn) {
        return {false, "Not player turn"};
    }

    if (!state.hand.contains(instance.instanceId)) {
        return {false, "Card is not in hand"};
    }

    if (!state.resources.canSpendEnergy(source, definition.energyCost)) {
        return {false, "Not enough energy"};
    }

    const bool unplayable = std::find(
        definition.keywords.begin(),
        definition.keywords.end(),
        CardKeyword::Unplayable
    ) != definition.keywords.end();

    if (unplayable) {
        return {false, "Card is unplayable"};
    }

    return {true, {}};
}
