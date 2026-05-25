#include "CardRarity.hpp"

#include <stdexcept>

std::string toString(const CardRarity rarity) {
    switch (rarity) {
        case CardRarity::Starter:
            return "starter";
        case CardRarity::Common:
            return "common";
        case CardRarity::Uncommon:
            return "uncommon";
        case CardRarity::Rare:
            return "rare";
        case CardRarity::Special:
            return "special";
    }

    throw std::runtime_error("Unknown CardRarity");
}

CardRarity cardRarityFromString(const std::string_view value) {
    if (value == "starter") {
        return CardRarity::Starter;
    }

    if (value == "common") {
        return CardRarity::Common;
    }

    if (value == "uncommon") {
        return CardRarity::Uncommon;
    }

    if (value == "rare") {
        return CardRarity::Rare;
    }

    if (value == "special") {
        return CardRarity::Special;
    }

    throw std::runtime_error("Unknown card rarity: " + std::string(value));
}
