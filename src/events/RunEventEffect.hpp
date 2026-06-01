#pragma once

#include <string>

enum class RunEventEffectType {
    GainGold,
    LoseGold,
    GainCard,
    GainRandomCard,
    GainRelic,
    GainRandomRelic,
    GainConsumable,
    GainRandomConsumable,
    RemoveCard,
    RemoveRandomCard,
    GainStress,
    LoseStress,
    LoseHp,
    HealAll,
    Skip
};

struct RunEventEffect {
    RunEventEffectType type = RunEventEffectType::Skip;
    int amount = 0;

    // Optional concrete content id used by explicit event effects:
    // gain_card/card_id, gain_relic/relic_id, gain_consumable/consumable_id, remove_card/card_id.
    std::string contentId;
};
