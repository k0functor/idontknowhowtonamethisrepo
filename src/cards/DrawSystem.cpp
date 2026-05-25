#include "DrawSystem.hpp"

std::size_t DrawSystem::drawCards(
    Deck& deck,
    Hand& hand,
    const std::size_t amount,
    Random& random
) const {
    std::size_t drawn = 0;

    while (drawn < amount) {
        if (deck.drawPile.empty()) {
            recycleDiscardIntoDrawPile(deck, random);
        }

        if (deck.drawPile.empty()) {
            break;
        }

        hand.add(deck.drawPile.drawTop());
        ++drawn;
    }

    return drawn;
}

void DrawSystem::recycleDiscardIntoDrawPile(Deck& deck, Random& random) const {
    while (!deck.discardPile.empty()) {
        deck.drawPile.addTop(deck.discardPile.drawTop());
    }

    deck.drawPile.shuffle(random);
}
