#pragma once

#include "combat/CombatState.hpp"

class EnergySystem {
public:
    bool canSpend(const CombatState& state, int amount) const;
    void spend(CombatState& state, int amount) const;
    void gain(CombatState& state, int amount) const;
};
