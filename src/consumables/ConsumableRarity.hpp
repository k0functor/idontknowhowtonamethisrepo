#pragma once

#include <string>
#include <string_view>

enum class ConsumableRarity {
    Common,
    Uncommon,
    Rare,
    Special
};

std::string toString(ConsumableRarity rarity);
ConsumableRarity consumableRarityFromString(std::string_view value);
