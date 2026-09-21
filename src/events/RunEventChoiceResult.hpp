#pragma once

#include <string>
#include <vector>

enum class RunEventOutcomeType {
    GoldGained,
    GoldLost,
    CardGained,
    CardRemoved,
    CardUpgraded,
    RelicGained,
    ConsumableGained,
    StressGained,
    StressLost,
    HpLost,
    HpHealed,
    Nothing
};

struct RunEventOutcomeEntry {
    RunEventOutcomeType type = RunEventOutcomeType::Nothing;
    int amount = 0;
    std::string contentId;
};

struct RunEventChoiceResult {
    bool completed = false;
    std::vector<RunEventOutcomeEntry> outcomes;
};
