#include "RelicModifierDefinition.hpp"

#include <stdexcept>

std::string toString(const RelicModifierType type) {
    switch (type) {
        case RelicModifierType::GoldRewardMultiply:
            return "gold_reward_multiply";
    }

    throw std::runtime_error("Unknown RelicModifierType");
}

RelicModifierType relicModifierTypeFromString(const std::string_view value) {
    if (value == "gold_reward_multiply") return RelicModifierType::GoldRewardMultiply;

    throw std::runtime_error(
        "Unknown relic modifier type: " + std::string(value) +
        ". Combat values must be modified through statuses."
    );
}
