#pragma once

#include "cards/CardId.hpp"
#include "cards/CardKeyword.hpp"
#include "cards/CardRarity.hpp"
#include "cards/CardType.hpp"
#include "dice/DiceCorruption.hpp"
#include "effects/EffectDefinition.hpp"
#include "localization/TextId.hpp"

#include <optional>
#include <string>
#include <vector>

struct CardUpgradeDefinition {
    std::optional<TextId> nameTextId;
    std::optional<TextId> descriptionTextId;
    std::optional<int> energyCost;
    std::optional<int> goldCost;
    std::optional<std::vector<CardKeyword>> keywords;
    std::optional<DiceCorruption> diceCorruption;
    std::optional<std::vector<EffectDefinition>> effects;

    bool empty() const;
};

struct CardDefinition {
    CardId id;

    TextId nameTextId;
    TextId descriptionTextId;

    CardRarity rarity = CardRarity::Common;
    CardType type = CardType::Attack;

    int energyCost = 0;
    int goldCost = 0;

    // Empty means: use the default/first player actor.
    // Sadist/Masochist and other multi-actor archetypes use this to route
    // card source, modifiers and actor-specific energy.
    std::string ownerActorId;

    std::vector<CardKeyword> keywords;

    DiceCorruption diceCorruption;

    std::vector<EffectDefinition> effects;

    CardUpgradeDefinition upgrade;
};
