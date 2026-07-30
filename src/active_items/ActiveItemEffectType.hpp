#pragma once

#include <string>

enum class ActiveItemEffectType {
    HealParty,
    GainGold,
    RerollOffers,
    SkipEnemyTurn,
    CreateConsumable,
    RerollMapChoices,
    CopyCard,
    StabilizeStress
};

ActiveItemEffectType activeItemEffectTypeFromString(const std::string& value);
std::string toString(ActiveItemEffectType type);
