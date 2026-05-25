#pragma once

#include "cards/Deck.hpp"
#include "cards/Hand.hpp"
#include "core/Random.hpp"

#include <cstddef>

class DrawSystem {
public:
    std::size_t drawCards(
        Deck& deck,
        Hand& hand,
        std::size_t amount,
        Random& random
    ) const;

private:
    void recycleDiscardIntoDrawPile(Deck& deck, Random& random) const;
};
