#pragma once

enum class RunEventEffectType {
    GainGold,
    LoseGold,
    GainRandomCard,
    GainRandomRelic,
    GainRandomConsumable,
    GainStress,
    LoseStress,
    LoseHp,
    HealAll,
    Skip
};

struct RunEventEffect {
    RunEventEffectType type = RunEventEffectType::Skip;
    int amount = 0;
};
