#include "EnergySystem.hpp"

#include <algorithm>

bool EnergySystem::canSpend(const CombatState& state, const int amount) const {
    return state.resources.canSpendEnergy(amount);
}

void EnergySystem::spend(CombatState& state, const int amount) const {
    state.resources.spendEnergy(amount);
}

void EnergySystem::gain(CombatState& state, const int amount) const {
    state.resources.gainEnergy(amount);
}

int EnergySystem::lose(CombatState& state, const int amount) const {
    if (amount <= 0) {
        return 0;
    }

    const int lost = std::min(state.resources.energy(), amount);
    if (lost > 0) {
        state.resources.spendEnergy(lost);
    }
    return lost;
}

bool EnergySystem::canSpend(const CombatState& state, const EntityId owner, const int amount) const {
    return state.resources.canSpendEnergy(owner, amount);
}

void EnergySystem::spend(CombatState& state, const EntityId owner, const int amount) const {
    state.resources.spendEnergy(owner, amount);
}

void EnergySystem::gain(CombatState& state, const EntityId owner, const int amount) const {
    state.resources.gainEnergy(owner, amount);
}

int EnergySystem::lose(CombatState& state, const EntityId owner, const int amount) const {
    if (amount <= 0) {
        return 0;
    }

    const int lost = std::min(state.resources.energyFor(owner), amount);
    if (lost > 0) {
        state.resources.spendEnergy(owner, lost);
    }
    return lost;
}
