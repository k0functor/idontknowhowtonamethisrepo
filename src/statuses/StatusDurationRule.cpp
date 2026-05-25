#include "StatusDurationRule.hpp"

#include <stdexcept>

std::string toString(const StatusDurationRule rule) {
    switch (rule) {
        case StatusDurationRule::PersistentCombat:
            return "persistent_combat";
        case StatusDurationRule::DecreaseEndOfOwnerTurn:
            return "decrease_end_of_owner_turn";
        case StatusDurationRule::Custom:
            return "custom";
    }

    throw std::runtime_error("Unknown StatusDurationRule");
}

StatusDurationRule statusDurationRuleFromString(const std::string_view value) {
    if (value == "persistent_combat") {
        return StatusDurationRule::PersistentCombat;
    }

    if (value == "decrease_end_of_owner_turn") {
        return StatusDurationRule::DecreaseEndOfOwnerTurn;
    }

    if (value == "custom") {
        return StatusDurationRule::Custom;
    }

    throw std::runtime_error("Unknown status duration rule: " + std::string(value));
}
