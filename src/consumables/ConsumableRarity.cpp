#include "ConsumableRarity.hpp"

#include <stdexcept>

std::string toString(const ConsumableRarity rarity) {
    switch (rarity) {
        case ConsumableRarity::Common: return "common";
        case ConsumableRarity::Uncommon: return "uncommon";
        case ConsumableRarity::Rare: return "rare";
        case ConsumableRarity::Special: return "special";
    }

    throw std::runtime_error("Unknown ConsumableRarity");
}

ConsumableRarity consumableRarityFromString(const std::string_view value) {
    if (value == "common" || value == "Common") return ConsumableRarity::Common;
    if (value == "uncommon" || value == "Uncommon") return ConsumableRarity::Uncommon;
    if (value == "rare" || value == "Rare") return ConsumableRarity::Rare;
    if (value == "special" || value == "Special") return ConsumableRarity::Special;

    throw std::runtime_error("Unknown consumable rarity: " + std::string(value));
}
