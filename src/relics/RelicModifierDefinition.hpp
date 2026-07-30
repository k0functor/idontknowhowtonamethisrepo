#pragma once

#include "effects/EffectType.hpp"

#include <string>
#include <string_view>

enum class RelicModifierType {
    GoldRewardMultiply
};

std::string toString(RelicModifierType type);
RelicModifierType relicModifierTypeFromString(std::string_view value);

struct RelicModifierDefinition {
    RelicModifierType type = RelicModifierType::GoldRewardMultiply;

    int amount = 0;
    double multiplier = 1.0;

    // Optional: when true, applies only to player-side calculations in combat.
    bool playerOnly = true;

    int priority = 500;
};
