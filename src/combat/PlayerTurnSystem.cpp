#include "PlayerTurnSystem.hpp"

#include "cards/CardKeyword.hpp"
#include "cards/CardUpgrade.hpp"
#include "run/StressRules.hpp"

#include <algorithm>
#include <string>
#include <utility>
#include <vector>

PlayerTurnSystem::PlayerTurnSystem(
    const DrawSystem& drawSystem,
    const CardDatabase& cardDatabase
)
    : drawSystem_(drawSystem),
      cardDatabase_(cardDatabase) {}

void PlayerTurnSystem::startTurn(
    CombatState& state,
    const std::size_t handSize,
    Random& random
) const {
    state.phase = CombatPhase::PlayerTurn;
    state.resources.resetEnergy();

    for (CombatEntity& player : state.players) {
        player.block = 0;
    }

    drawSystem_.drawCards(state.deck, state.hand, handSize, random);
    state.log.add(CombatLogEntryType::PlayerTurnStarted, {{"turn", std::to_string(state.turn)}});

    applyStartOfTurnTraitEffects(state, random);
}

void PlayerTurnSystem::applyStartOfTurnTraitEffects(CombatState& state, Random& random) const {
    for (const CombatEntity& player : state.players) {
        if (!player.isAlive()) {
            continue;
        }

        if (!StressRules::hasTrait(player, StressRules::BreakdownTraitId)) {
            continue;
        }

        if (state.hand.empty()) {
            return;
        }

        const int randomIndex = random.rangeInclusive(0, static_cast<int>(state.hand.size() - 1u));
        std::vector<CardInstance>& cards = state.hand.cards();
        CardInstance discarded = std::move(cards[static_cast<std::size_t>(randomIndex)]);
        cards.erase(cards.begin() + randomIndex);

        const std::string discardedCardId = discarded.definitionId.value;
        state.deck.discardPile.addTop(std::move(discarded));
        state.log.add(
            CombatLogEntryType::StressBreakdownDiscard,
            {
                {"actor", player.definitionId},
                {"actor_text_id", player.nameTextId.value},
                {"card", discardedCardId}
            }
        );
    }
}

void PlayerTurnSystem::endTurn(CombatState& state) const {
    discardHand(state);
    state.log.add(CombatLogEntryType::PlayerTurnEnded);
}

void PlayerTurnSystem::discardHand(CombatState& state) const {
    std::vector<CardInstance> cards = std::move(state.hand.cards());
    state.hand.clear();

    for (CardInstance& card : cards) {
        const CardDefinition definition = CardUpgrade::effectiveDefinition(cardDatabase_.get(card.definitionId), card.upgraded);
        const bool retains = std::find(
            definition.keywords.begin(),
            definition.keywords.end(),
            CardKeyword::Retain
        ) != definition.keywords.end();

        if (retains) {
            state.hand.add(std::move(card));
        } else {
            state.deck.discardPile.addTop(std::move(card));
        }
    }
}
