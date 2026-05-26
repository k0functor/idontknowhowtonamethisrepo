#pragma once

#include "cards/CardDefinition.hpp"
#include "cards/CardInstance.hpp"
#include "combat/CombatPhase.hpp"
#include "combat/CombatState.hpp"
#include "entities/EntityId.hpp"

#include <string>

struct CardPlayValidationResult {
    bool valid = false;
    std::string reason;
};

class CardPlayValidator {
public:
    CardPlayValidationResult validate(
        const CombatState& state,
        const CardDefinition& definition,
        const CardInstance& instance
    ) const;

    CardPlayValidationResult validate(
        const CombatState& state,
        const CardDefinition& definition,
        const CardInstance& instance,
        EntityId source
    ) const;
};
