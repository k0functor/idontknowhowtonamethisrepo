#pragma once

#include "localization/TextId.hpp"
#include "statuses/StatusDurationRule.hpp"
#include "statuses/StatusId.hpp"
#include "statuses/StatusType.hpp"

#include <string>

struct StatusDefinition {
    StatusId id;

    TextId nameTextId;
    TextId descriptionTextId;

    StatusType type = StatusType::Neutral;
    StatusDurationRule durationRule = StatusDurationRule::PersistentCombat;

    // Current v1 special hook. Later this should become a data-driven trigger/effect list.
    std::string endTurnEffect;
    bool decreaseAfterTrigger = false;
};
