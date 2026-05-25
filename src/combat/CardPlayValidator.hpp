#pragma once

#include "cards/CardDefinition.hpp"
#include "cards/CardInstance.hpp"
#include "combat/CombatState.hpp"

#include <string>

struct CardPlayValidationResult {
    bool valid = true;
    std::string reason;
};

class CardPlayValidator {
public:
    CardPlayValidationResult validate(
        const CombatState& state,
        const CardDefinition& definition,
        const CardInstance& instance
    ) const;
};
