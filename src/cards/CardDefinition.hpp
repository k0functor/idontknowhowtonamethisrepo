#pragma once

#include "cards/CardId.hpp"
#include "cards/CardKeyword.hpp"
#include "cards/CardRarity.hpp"
#include "cards/CardType.hpp"
#include "dice/DiceCorruption.hpp"
#include "effects/EffectDefinition.hpp"
#include "localization/TextId.hpp"

#include <vector>

struct CardDefinition {
    CardId id;

    TextId nameTextId;
    TextId descriptionTextId;

    CardRarity rarity = CardRarity::Common;
    CardType type = CardType::Attack;

    int energyCost = 0;
    int goldCost = 0;

    std::vector<CardKeyword> keywords;

    DiceCorruption diceCorruption;

    std::vector<EffectDefinition> effects;
};
