#pragma once

#include <string>
#include <string_view>

enum class StatusDurationRule {
    PersistentCombat,
    DecreaseEndOfOwnerTurn,
    Custom
};

std::string toString(StatusDurationRule rule);

StatusDurationRule statusDurationRuleFromString(std::string_view value);
