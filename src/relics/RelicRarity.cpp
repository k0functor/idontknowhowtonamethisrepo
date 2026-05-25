#include "RelicRarity.hpp"

#include <stdexcept>

std::string toString(const RelicRarity rarity) {
    switch (rarity) {
        case RelicRarity::Starter:
            return "starter";
        case RelicRarity::Common:
            return "common";
        case RelicRarity::Uncommon:
            return "uncommon";
        case RelicRarity::Rare:
            return "rare";
        case RelicRarity::Boss:
            return "boss";
        case RelicRarity::Special:
            return "special";
    }

    throw std::runtime_error("Unknown RelicRarity");
}

RelicRarity relicRarityFromString(const std::string_view value) {
    if (value == "starter") return RelicRarity::Starter;
    if (value == "common") return RelicRarity::Common;
    if (value == "uncommon") return RelicRarity::Uncommon;
    if (value == "rare") return RelicRarity::Rare;
    if (value == "boss") return RelicRarity::Boss;
    if (value == "special") return RelicRarity::Special;

    throw std::runtime_error("Unknown relic rarity: " + std::string(value));
}
