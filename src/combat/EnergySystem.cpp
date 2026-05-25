#include "EnergySystem.hpp"

bool EnergySystem::canSpend(const CombatState& state, const int amount) const {
    return state.resources.canSpendEnergy(amount);
}

void EnergySystem::spend(CombatState& state, const int amount) const {
    state.resources.spendEnergy(amount);
}

void EnergySystem::gain(CombatState& state, const int amount) const {
    state.resources.gainEnergy(amount);
}
