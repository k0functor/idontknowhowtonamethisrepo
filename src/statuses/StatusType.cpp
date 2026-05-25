#include "StatusType.hpp"

#include <stdexcept>

std::string toString(const StatusType type) {
    switch (type) {
        case StatusType::Buff:
            return "buff";
        case StatusType::Debuff:
            return "debuff";
        case StatusType::Neutral:
            return "neutral";
    }

    throw std::runtime_error("Unknown StatusType");
}

StatusType statusTypeFromString(const std::string_view value) {
    if (value == "buff") {
        return StatusType::Buff;
    }

    if (value == "debuff") {
        return StatusType::Debuff;
    }

    if (value == "neutral") {
        return StatusType::Neutral;
    }

    throw std::runtime_error("Unknown status type: " + std::string(value));
}
