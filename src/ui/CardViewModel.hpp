#pragma once

#include "cards/CardInstanceId.hpp"
#include "cards/CardRarity.hpp"
#include "cards/CardType.hpp"

#include <string>

struct CardViewModel {
    CardInstanceId instanceId;

    std::string name;
    std::string description;

    int energyCost = 0;

    CardType type = CardType::Attack;
    CardRarity rarity = CardRarity::Common;

    bool playable = true;
    bool selected = false;
};
