#include "CombatResources.hpp"

#include <algorithm>
#include <stdexcept>

int CombatResources::energy() const {
    if (!actorEnergy_.empty()) {
        int total = 0;

        for (const auto& [_, value] : actorEnergy_) {
            total += value;
        }

        return total;
    }

    return energy_;
}

int CombatResources::maxEnergy() const {
    if (!actorMaxEnergy_.empty()) {
        int total = 0;

        for (const auto& [_, value] : actorMaxEnergy_) {
            total += value;
        }

        return total;
    }

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

    for (const auto& [owner, maximum] : actorMaxEnergy_) {
        actorEnergy_[owner] = maximum;
    }
}

bool CombatResources::canSpendEnergy(const int amount) const {
    return amount >= 0 && energy() >= amount;
}

void CombatResources::spendEnergy(const int amount) {
    if (amount < 0) {
        throw std::runtime_error("Cannot spend negative energy");
    }

    if (!canSpendEnergy(amount)) {
        throw std::runtime_error("Not enough energy");
    }

    if (!actorEnergy_.empty()) {
        int remaining = amount;

        for (auto& [_, value] : actorEnergy_) {
            const int spent = std::min(value, remaining);
            value -= spent;
            remaining -= spent;

            if (remaining == 0) {
                return;
            }
        }

        return;
    }

    energy_ -= amount;
}

void CombatResources::gainEnergy(const int amount) {
    if (amount <= 0) {
        return;
    }

    energy_ += amount;
}

void CombatResources::clearActorEnergyPools() {
    actorEnergy_.clear();
    actorMaxEnergy_.clear();
}

void CombatResources::setMaxEnergyFor(const EntityId owner, const int value) {
    if (value < 0) {
        throw std::runtime_error("Actor max energy must not be negative");
    }

    actorMaxEnergy_[owner.value] = value;
    actorEnergy_[owner.value] = value;
}

bool CombatResources::hasActorEnergyPool(const EntityId owner) const {
    return actorMaxEnergy_.contains(owner.value);
}

int CombatResources::energyFor(const EntityId owner) const {
    const auto iterator = actorEnergy_.find(owner.value);

    if (iterator == actorEnergy_.end()) {
        return energy_;
    }

    return iterator->second;
}

int CombatResources::maxEnergyFor(const EntityId owner) const {
    const auto iterator = actorMaxEnergy_.find(owner.value);

    if (iterator == actorMaxEnergy_.end()) {
        return maxEnergy_;
    }

    return iterator->second;
}

bool CombatResources::canSpendEnergy(const EntityId owner, const int amount) const {
    if (amount < 0) {
        return false;
    }

    if (!hasActorEnergyPool(owner)) {
        return canSpendEnergy(amount);
    }

    return energyFor(owner) >= amount;
}

void CombatResources::spendEnergy(const EntityId owner, const int amount) {
    if (amount < 0) {
        throw std::runtime_error("Cannot spend negative energy");
    }

    if (!hasActorEnergyPool(owner)) {
        spendEnergy(amount);
        return;
    }

    if (!canSpendEnergy(owner, amount)) {
        throw std::runtime_error("Not enough actor energy");
    }

    actorEnergy_[owner.value] -= amount;
}

void CombatResources::gainEnergy(const EntityId owner, const int amount) {
    if (amount <= 0) {
        return;
    }

    if (!hasActorEnergyPool(owner)) {
        gainEnergy(amount);
        return;
    }

    actorEnergy_[owner.value] += amount;
}

void CombatResources::clearActorEnergy() {
    clearActorEnergyPools();
}

void CombatResources::setMaxEnergy(const EntityId owner, const int value) {
    setMaxEnergyFor(owner, value);
}
