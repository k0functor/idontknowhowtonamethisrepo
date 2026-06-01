#pragma once

#include "run/RunState.hpp"

#include <string>
#include <vector>

struct RunEventChoiceRequirements {
    int minGold = 0;
    int minHp = 0;
    bool freeConsumableSlot = false;
    int minDeckSize = 0;
    std::vector<std::string> requiredRelicIds;
    std::vector<std::string> forbiddenRelicIds;
    std::vector<std::string> requiredCardIds;
    std::vector<std::string> forbiddenCardIds;
};

enum class RunEventChoiceBlockReasonType {
    NotEnoughGold,
    NotEnoughHp,
    NoFreeConsumableSlot,
    NotEnoughCards,
    MissingRequiredRelic,
    HasForbiddenRelic,
    MissingRequiredCard,
    HasForbiddenCard
};

struct RunEventChoiceBlockReason {
    RunEventChoiceBlockReasonType type = RunEventChoiceBlockReasonType::NotEnoughGold;
    int required = 0;
    int current = 0;
    std::string id;
};

struct RunEventChoiceAvailability {
    bool available = true;
    std::vector<RunEventChoiceBlockReason> reasons;
};

RunEventChoiceAvailability evaluateRunEventChoiceRequirements(
    const RunEventChoiceRequirements& requirements,
    const RunState& state
);
