#pragma once

#include <string>
#include <string_view>

enum class EffectType {
    Damage,
    Block,
    Heal,
    DrawCards,
    DiscardCards,
    RecoverCards,
    ApplyStatus,
    GainEnergy,
    GainStress,
    LoseEnergy,
    LoseStress,
    SpendStressDamage,
    SpendStressBlock,
    SpendStressEnergy,
    SpendStressDraw,
    PrimeStressBreakdown,
    LoseHp,
    EnterStance,
    SummonDrone,
    UseDrone
};

std::string toString(const EffectType& effect);

EffectType effectTypeFromString(std::string_view value);

bool isStressConversionEffect(EffectType effect);
