#pragma once

#include "active_items/ActiveItemEffectType.hpp"
#include "active_items/ActiveItemId.hpp"
#include "active_items/ActiveItemUseContext.hpp"
#include "localization/TextId.hpp"

#include <vector>

struct ActiveItemEffectDefinition {
    ActiveItemEffectType type = ActiveItemEffectType::HealParty;
    int amount = 0;
};

struct ActiveItemDefinition {
    ActiveItemId id;
    TextId nameTextId;
    TextId descriptionTextId;
    int maxCharge = 1;
    int chargeCost = 1;
    int startingCharge = 0;
    int combatCharge = 1;
    int eliteCharge = 2;
    int bossCharge = 3;
    int shopPrice = 0;
    bool canAppearInRewards = false;
    bool canAppearInShop = false;
    std::vector<ActiveItemUseContext> useContexts;
    std::vector<ActiveItemEffectDefinition> effects;
};
