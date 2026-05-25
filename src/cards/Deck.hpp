#pragma once

#include "cards/CardPile.hpp"

struct Deck {
    CardPile drawPile;
    CardPile discardPile;
    CardPile exhaustPile;

    void clear();
};
