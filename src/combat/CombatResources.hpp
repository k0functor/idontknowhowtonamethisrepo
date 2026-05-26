#pragma once

#include "entities/EntityId.hpp"

#include <cstdint>
#include <unordered_map>

class CombatResources {
public:
    int energy() const;
    int maxEnergy() const;

    void setMaxEnergy(int value);
    void resetEnergy();

    bool canSpendEnergy(int amount) const;
    void spendEnergy(int amount);
    void gainEnergy(int amount);

    void clearActorEnergyPools();
    void setMaxEnergyFor(EntityId owner, int value);
    bool hasActorEnergyPool(EntityId owner) const;
    int energyFor(EntityId owner) const;
    int maxEnergyFor(EntityId owner) const;

    bool canSpendEnergy(EntityId owner, int amount) const;
    void spendEnergy(EntityId owner, int amount);
    void gainEnergy(EntityId owner, int amount);

    // Compatibility aliases for older/newer patch layers.
    // Some CombatScene versions call these names directly.
    void clearActorEnergy();
    void setMaxEnergy(EntityId owner, int value);

private:
    int energy_ = 3;
    int maxEnergy_ = 3;

    std::unordered_map<std::uint64_t, int> actorEnergy_;
    std::unordered_map<std::uint64_t, int> actorMaxEnergy_;
};
