#pragma once

#include <string>
#include <string_view>

enum class RelicRarity {
    Starter,
    Common,
    Uncommon,
    Rare,
    Boss,
    Special
};

std::string toString(RelicRarity rarity);
RelicRarity relicRarityFromString(std::string_view value);
