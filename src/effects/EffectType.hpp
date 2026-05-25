#pragma once

#include <string>
#include <string_view>

enum class EffectType {
    Damage,
    Block,
    Heal,
    DrawCards,
    DiscardCards,
    ApplyStatus,
    GainEnergy,
    GainStress,
    LoseEnergy,
    LoseStress,
    LoseHp
};

std::string toString(const EffectType& effect);

EffectType effectTypeFromString(std::string_view value);
