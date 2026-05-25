#include "PlayerTurnSystem.hpp"

#include <string>
#include <utility>
#include <vector>

PlayerTurnSystem::PlayerTurnSystem(const DrawSystem& drawSystem)
    : drawSystem_(drawSystem) {}

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
    state.log.add("Player turn started: " + std::to_string(state.turn));
}

void PlayerTurnSystem::endTurn(CombatState& state) const {
    discardHand(state);
    state.log.add("Player turn ended");
}

void PlayerTurnSystem::discardHand(CombatState& state) const {
    std::vector<CardInstance> cards = std::move(state.hand.cards());
    state.hand.clear();

    for (CardInstance& card : cards) {
        state.deck.discardPile.addTop(std::move(card));
    }
}
