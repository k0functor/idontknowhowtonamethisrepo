#include "CardPlaySystem.hpp"

#include "cards/CardKeyword.hpp"
#include "combat/CardCost.hpp"

#include <algorithm>
#include <stdexcept>

CardPlaySystem::CardPlaySystem(
    const CardDatabase& cardDatabase,
    const CardPlayValidator& validator,
    const EnergySystem& energySystem,
    const EffectSystem& effectSystem,
    const GameEventBus* eventBus
)
    : cardDatabase_(cardDatabase),
      validator_(validator),
      energySystem_(energySystem),
      effectSystem_(effectSystem),
      eventBus_(eventBus) {}

PlayCardResult CardPlaySystem::playCard(
    CombatState& state,
    const PlayCardRequest& request,
    Random& random
) const {
    if (!state.hand.contains(request.cardInstanceId)) {
        return {false, "Card is not in hand"};
    }

    const CardInstance& instanceInHand = state.hand.get(request.cardInstanceId);
    const CardDefinition& definition = cardDatabase_.get(instanceInHand.definitionId);

    const CardPlayValidationResult validation = validator_.validate(
        state,
        definition,
        instanceInHand,
        request.source
    );

    if (!validation.valid) {
        return {false, validation.reason};
    }

    const int energyCost = CardCost::effectiveEnergyCost(state, request.source, definition);
    energySystem_.spend(state, request.source, energyCost);
    CardCost::consumeFreeNextCard(state, request.source, definition);

    EffectContext context;
    context.source = request.source;
    context.explicitTarget = request.target;
    context.cardInstanceId = request.cardInstanceId;
    context.cardDefinitionId = definition.id;
    context.diceCorruption = definition.diceCorruption;
    context.random = &random;

    effectSystem_.applyEffects(state, definition.effects, context);

    std::optional<CardInstance> removedCard = state.hand.remove(request.cardInstanceId);
    if (!removedCard.has_value()) {
        throw std::runtime_error("Played card disappeared from hand");
    }

    const bool exhaustByKeyword = std::find(
        definition.keywords.begin(),
        definition.keywords.end(),
        CardKeyword::Exhaust
    ) != definition.keywords.end();

    if (exhaustByKeyword || removedCard->markedForExhaust) {
        state.deck.exhaustPile.addTop(std::move(*removedCard));
    } else {
        state.deck.discardPile.addTop(std::move(*removedCard));
    }

    state.log.add("Played card: " + definition.id.value);

    if (eventBus_ != nullptr) {
        GameEvent event;
        event.type = GameEventType::CardPlayed;
        event.source = request.source;
        event.target = request.target;
        event.cardInstanceId = request.cardInstanceId;
        event.cardDefinitionId = definition.id;
        event.turn = state.turn;
        eventBus_->emit(event);
    }

    return {true, {}};
}
