#pragma once

#include "cards/CardInstanceId.hpp"
#include "cards/CardRarity.hpp"
#include "cards/CardType.hpp"

#include <string>

struct CardViewModel {
    CardInstanceId instanceId;

    std::string name;
    std::string description;
    std::string ownerLabel;
    std::string sourceActorName;
    std::string sourceEnergyLabel;
    std::string unplayableReason;

    int energyCost = 0;
    int stressCost = 0;
    int sourceCurrentStress = 0;
    int sourceMaxStress = 0;
    int sourceStressAfterMinimum = 0;
    int sourceStressAfterMaximum = 0;
    bool wouldCollapseFromStress = false;
    std::string stressPreviewLabel;

    bool previewRequiresTarget = false;

    int sourceCurrentEnergy = 0;
    int sourceMaxEnergy = 0;

    bool sharedSource = false;
    bool sourceCanPay = true;

    CardType type = CardType::Attack;
    CardRarity rarity = CardRarity::Common;

    bool playable = true;
    bool selected = false;
    bool upgraded = false;
};
