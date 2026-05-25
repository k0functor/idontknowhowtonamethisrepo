#pragma once

#include "localization/TextId.hpp"
#include "relics/RelicId.hpp"
#include "relics/RelicModifierDefinition.hpp"
#include "relics/RelicRarity.hpp"
#include "relics/RelicTriggerDefinition.hpp"

#include <string>
#include <vector>

struct RelicDefinition {
    RelicId id;

    TextId nameTextId;
    TextId descriptionTextId;

    RelicRarity rarity = RelicRarity::Common;

    std::string mechanicId = "default";

    std::vector<RelicModifierDefinition> modifiers;
    std::vector<RelicTriggerDefinition> triggers;
};
