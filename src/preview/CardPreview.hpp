#pragma once

#include "cards/CardId.hpp"
#include "cards/CardInstanceId.hpp"
#include "preview/EffectPreview.hpp"

#include <string>
#include <vector>

struct CardPreview {
    CardInstanceId cardInstanceId;
    CardId cardDefinitionId;

    bool playable = true;
    std::string unplayableReason;

    int energyCost = 0;

    std::vector<EffectPreview> effects;
};
