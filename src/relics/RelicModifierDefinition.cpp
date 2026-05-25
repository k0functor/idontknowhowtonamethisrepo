#include "RelicModifierDefinition.hpp"

#include <stdexcept>

std::string toString(const RelicModifierType type) {
    switch (type) {
        case RelicModifierType::OutgoingDamageAdd:
            return "outgoing_damage_add";
        case RelicModifierType::OutgoingDamageMultiply:
            return "outgoing_damage_multiply";
        case RelicModifierType::BlockAdd:
            return "block_add";
        case RelicModifierType::GoldRewardMultiply:
            return "gold_reward_multiply";
    }

    throw std::runtime_error("Unknown RelicModifierType");
}

RelicModifierType relicModifierTypeFromString(const std::string_view value) {
    if (value == "outgoing_damage_add") return RelicModifierType::OutgoingDamageAdd;
    if (value == "outgoing_damage_multiply") return RelicModifierType::OutgoingDamageMultiply;
    if (value == "block_add") return RelicModifierType::BlockAdd;
    if (value == "gold_reward_multiply") return RelicModifierType::GoldRewardMultiply;

    throw std::runtime_error("Unknown relic modifier type: " + std::string(value));
}
