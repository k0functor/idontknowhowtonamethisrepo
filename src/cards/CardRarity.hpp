#pragma once

#include <string>
#include <string_view>

enum class CardRarity {
    Starter,
    Common,
    Uncommon,
    Rare,
    Special
};

std::string toString(CardRarity rarity);

CardRarity cardRarityFromString(std::string_view value);
