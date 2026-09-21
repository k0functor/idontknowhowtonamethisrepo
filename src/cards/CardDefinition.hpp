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
    std::optional<int> stressCost;
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
    int stressCost = 0;
    int goldCost = 0;

    // Empty means: playable by the current active player actor.
    // Non-empty means the card is personal and can only be played by that actor.
    std::string ownerActorId;

    // Empty means: use ownerActorId as the reward-pool key. Shared cards can
    // use this without becoming personal cards.
    std::string rewardPoolId;

    std::vector<CardKeyword> keywords;

    DiceCorruption diceCorruption;

    std::vector<EffectDefinition> effects;

    CardUpgradeDefinition upgrade;
};
