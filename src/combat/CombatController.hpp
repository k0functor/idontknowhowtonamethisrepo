#pragma once

#include "combat/CombatOutcome.hpp"
#include "combat/CombatResult.hpp"
#include "combat/CombatState.hpp"

class CombatController {
public:
    CombatOutcome checkOutcome(const CombatState& state) const;
    CombatResult buildResult(const CombatState& state) const;

    CombatResult updateAfterAction(CombatState& state) const;

private:
    void applyOutcomeToState(CombatState& state, CombatOutcome outcome) const;
};
