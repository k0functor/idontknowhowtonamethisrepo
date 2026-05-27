#pragma once

enum class RunEventEffectType {
    GainGold,
    LoseGold,
    GainRandomCard,
    GainRandomRelic,
    GainRandomConsumable,
    Skip
};

struct RunEventEffect {
    RunEventEffectType type = RunEventEffectType::Skip;
    int amount = 0;
};
