#pragma once

#include "cards/CardId.hpp"
#include "cards/CardInstanceId.hpp"
#include "combat/CardPlayFailureReason.hpp"
#include "preview/CardOutcomePreview.hpp"
#include "preview/EffectPreview.hpp"

#include <string>
#include <vector>

struct CardPreview {
    CardInstanceId cardInstanceId;
    CardId cardDefinitionId;

    bool playable = true;
    CardPlayFailureReason unplayableReasonCode = CardPlayFailureReason::None;
    std::string unplayableReason;

    int energyCost = 0;
    int stressCost = 0;
    int sourceStressBefore = 0;
    int sourceStressAfterMinimum = 0;
    int sourceStressAfterMaximum = 0;
    int sourceMaxStress = 0;
    bool wouldCollapseFromStress = false;

    CardOutcomePreview outcome;
    std::vector<EffectPreview> effects;
};
