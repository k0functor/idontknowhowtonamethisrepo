#include "CardPlaySystem.hpp"

#include "cards/CardKeyword.hpp"
#include "cards/CardUpgrade.hpp"
#include "combat/CardCost.hpp"
#include "combat/CardStressCost.hpp"
#include "run/StressRules.hpp"
#include "run/StressPsychopathRules.hpp"

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
    const CardDefinition definition = CardUpgrade::effectiveDefinition(cardDatabase_.get(instanceInHand.definitionId), instanceInHand.upgraded);

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

    const int directStressCost = CardStressCost::directCost(definition);
    if (directStressCost > 0) {
        CombatEntity& sourceEntity = state.entity(request.source);
        const StressRules::StressAdjustmentResult stressPayment =
            StressRules::applyDelta(
                sourceEntity,
                -directStressCost,
                &random,
                StressPsychopathRules::appliesTo(sourceEntity.definitionId)
            );
        state.log.add(
            CombatLogEntryType::LoseStress,
            {
                {"amount", std::to_string(-stressPayment.applied)},
                {"before", std::to_string(sourceEntity.stress - stressPayment.applied)},
                {"after", std::to_string(sourceEntity.stress)},
                {"target", sourceEntity.definitionId.empty() ? std::to_string(request.source.value) : sourceEntity.definitionId},
                {"target_text_id", sourceEntity.nameTextId.value},
                {"card", definition.id.value},
                {"reason", "card_cost"}
            }
        );
    }

    EffectContext context;
    context.source = request.source;
    context.explicitTarget = request.target;
    if (request.target.has_value() && state.hasEntity(*request.target)) {
        if (state.isEnemy(*request.target)) {
            context.explicitEnemyTarget = request.target;
        } else if (state.isPlayer(*request.target) && *request.target != request.source) {
            context.explicitAllyTarget = request.target;
        }
    }
    context.cardInstanceId = request.cardInstanceId;
    context.cardDefinitionId = definition.id;
    context.diceCorruption = definition.diceCorruption;
    context.usesActorStats = true;
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

    if (exhaustByKeyword || removedCard->markedForExhaust || removedCard->temporary) {
        state.deck.exhaustPile.addTop(std::move(*removedCard));
    } else {
        state.deck.discardPile.addTop(std::move(*removedCard));
    }

    state.playedCardIds.push_back(definition.id.value);
    ++state.telemetry.cardsPlayed;
    state.telemetry.energySpentOnCards += energyCost;
    state.log.add(CombatLogEntryType::CardPlayed, {{"card", definition.id.value}});

    if (eventBus_ != nullptr) {
        GameEvent event;
        event.type = GameEventType::CardPlayed;
        event.source = request.source;
        event.target = request.target;
        event.cardInstanceId = request.cardInstanceId;
        event.cardDefinitionId = definition.id;
        event.cardType = definition.type;
        event.turn = state.turn;
        eventBus_->emit(event);
    }

    return {true, {}};
}
