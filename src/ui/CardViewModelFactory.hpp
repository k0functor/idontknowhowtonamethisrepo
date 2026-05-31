#pragma once

#include "cards/CardDefinition.hpp"
#include "cards/CardInstanceId.hpp"
#include "localization/LocalizationManager.hpp"
#include "ui/CardViewModel.hpp"

namespace CardViewModelFactory {
CardViewModel buildStatic(
    const CardDefinition& definition,
    const LocalizationManager& localization,
    CardInstanceId instanceId = {},
    bool upgraded = false,
    bool selected = false,
    bool playable = true
);
}
