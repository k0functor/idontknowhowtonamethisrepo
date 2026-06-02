#pragma once

#include "combat/CombatState.hpp"
#include "entities/EntityId.hpp"

class EnergySystem {
public:
    bool canSpend(const CombatState& state, int amount) const;
    void spend(CombatState& state, int amount) const;
    void gain(CombatState& state, int amount) const;
    int lose(CombatState& state, int amount) const;

    bool canSpend(const CombatState& state, EntityId owner, int amount) const;
    void spend(CombatState& state, EntityId owner, int amount) const;
    void gain(CombatState& state, EntityId owner, int amount) const;
    int lose(CombatState& state, EntityId owner, int amount) const;
};
