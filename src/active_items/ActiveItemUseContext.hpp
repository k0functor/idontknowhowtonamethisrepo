#pragma once

#include <string>

// Contexts are intentionally broader than the first shipped item. The next
// active items can reroll reward/shop/chest content without changing save data.
enum class ActiveItemUseContext {
    Map,
    Combat,
    Reward,
    Shop,
    Chest,
    Event,
    Rest
};

ActiveItemUseContext activeItemUseContextFromString(const std::string& value);
std::string toString(ActiveItemUseContext context);
