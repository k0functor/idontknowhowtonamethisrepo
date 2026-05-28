#include "CardPlayValidator.hpp"

#include "cards/CardKeyword.hpp"
#include "combat/CardCost.hpp"

#include <algorithm>
#include <optional>
#include <utility>

namespace {
CardPlayValidationResult validResult() {
    return {true, CardPlayFailureReason::None, {}};
}

CardPlayValidationResult invalidResult(
    const CardPlayFailureReason reason,
    std::string message
) {
    return {false, reason, std::move(message)};
}
}

CardPlayValidationResult CardPlayValidator::validate(
    const CombatState& state,
    const CardDefinition& definition,
    const CardInstance& instance
) const {
    if (state.players.empty()) {
        return invalidResult(CardPlayFailureReason::NoPlayerActor, "No player actor");
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
        return invalidResult(CardPlayFailureReason::NotPlayerTurn, "Not player turn");
    }

    if (!state.hand.contains(instance.instanceId)) {
        return invalidResult(CardPlayFailureReason::CardNotInHand, "Card is not in hand");
    }

    if (!state.hasEntity(source) || !state.isPlayer(source)) {
        return invalidResult(CardPlayFailureReason::InvalidCardSource, "Invalid card source");
    }

    const CombatEntity& sourceEntity = state.entity(source);
    if (!sourceEntity.isAlive()) {
        return invalidResult(CardPlayFailureReason::CardSourceDefeated, "Card source is defeated");
    }

    if (state.useSequentialPlayerTurns) {
        const std::optional<EntityId> activePlayer = state.activePlayerId();
        if (!activePlayer.has_value() || *activePlayer != source) {
            return invalidResult(CardPlayFailureReason::WrongActorTurn, "Not this actor's turn");
        }
    }

    if (!definition.ownerActorId.empty() && sourceEntity.definitionId != definition.ownerActorId) {
        return invalidResult(CardPlayFailureReason::WrongActorForCard, "Wrong actor for card");
    }

    const int energyCost = CardCost::effectiveEnergyCost(state, source, definition);

    if (!state.resources.canSpendEnergy(source, energyCost)) {
        return invalidResult(CardPlayFailureReason::NotEnoughEnergy, "Not enough energy");
    }

    const bool unplayable = std::find(
        definition.keywords.begin(),
        definition.keywords.end(),
        CardKeyword::Unplayable
    ) != definition.keywords.end();

    if (unplayable) {
        return invalidResult(CardPlayFailureReason::UnplayableKeyword, "Card is unplayable");
    }

    return validResult();
}
