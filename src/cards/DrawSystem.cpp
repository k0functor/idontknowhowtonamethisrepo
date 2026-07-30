#include "DrawSystem.hpp"

#include "cards/CardKeyword.hpp"
#include "cards/CardUpgrade.hpp"

#include <algorithm>

namespace {
bool hasKeyword(const CardDefinition& definition, const CardKeyword keyword) {
    return std::find(definition.keywords.begin(), definition.keywords.end(), keyword) != definition.keywords.end();
}
}

std::size_t DrawSystem::drawCards(
    Deck& deck,
    Hand& hand,
    const std::size_t amount,
    Random& random
) const {
    std::size_t drawn = 0;

    while (drawn < amount && !hand.full()) {
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

std::size_t DrawSystem::drawOpeningHand(
    Deck& deck,
    Hand& hand,
    const CardDatabase& cardDatabase,
    const std::size_t normalHandSize,
    Random& random
) const {
    deck.drawPile.shuffle(random);

    std::size_t drawn = 0;
    std::vector<CardInstance>& cards = deck.drawPile.cards();
    for (std::size_t index = cards.size(); index > 0 && !hand.full();) {
        --index;
        const CardInstance& instance = cards[index];
        if (!cardDatabase.contains(instance.definitionId)) {
            continue;
        }

        const CardDefinition definition = CardUpgrade::effectiveDefinition(
            cardDatabase.get(instance.definitionId),
            instance.upgraded
        );
        if (!hasKeyword(definition, CardKeyword::Innate)) {
            continue;
        }

        CardInstance innate = std::move(cards[index]);
        cards.erase(cards.begin() + static_cast<std::ptrdiff_t>(index));
        hand.add(std::move(innate));
        ++drawn;
    }

    if (hand.size() < normalHandSize) {
        drawn += drawCards(deck, hand, normalHandSize - hand.size(), random);
    }

    return drawn;
}

void DrawSystem::recycleDiscardIntoDrawPile(Deck& deck, Random& random) const {
    while (!deck.discardPile.empty()) {
        deck.drawPile.addTop(deck.discardPile.drawTop());
    }

    deck.drawPile.shuffle(random);
}
