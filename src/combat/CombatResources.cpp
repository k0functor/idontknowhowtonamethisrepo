#include "CombatResources.hpp"

#include <algorithm>
#include <stdexcept>

int CombatResources::energy() const {
    return energy_;
}

int CombatResources::maxEnergy() const {
    return maxEnergy_;
}

void CombatResources::setMaxEnergy(const int value) {
    if (value < 0) {
        throw std::runtime_error("Combat max energy must not be negative");
    }

    maxEnergy_ = value;
    energy_ = std::min(energy_, maxEnergy_);
}

void CombatResources::resetEnergy() {
    energy_ = maxEnergy_;
}

bool CombatResources::canSpendEnergy(const int amount) const {
    return amount >= 0 && energy_ >= amount;
}

void CombatResources::spendEnergy(const int amount) {
    if (amount < 0) {
        throw std::runtime_error("Cannot spend negative energy");
    }

    if (!canSpendEnergy(amount)) {
        throw std::runtime_error("Not enough energy");
    }

    energy_ -= amount;
}

void CombatResources::gainEnergy(const int amount) {
    if (amount <= 0) {
        return;
    }

    energy_ += amount;
}
