#pragma once

#include "consumables/ConsumableId.hpp"
#include "consumables/ConsumableRarity.hpp"
#include "effects/EffectDefinition.hpp"
#include "localization/TextId.hpp"

#include <vector>

struct ConsumableDefinition {
    ConsumableId id;
    TextId nameTextId;
    TextId descriptionTextId;
    ConsumableRarity rarity = ConsumableRarity::Common;
    int goldCost = 0;
    std::vector<EffectDefinition> effects;
};
