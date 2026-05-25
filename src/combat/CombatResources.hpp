#pragma once

class CombatResources {
public:
    int energy() const;
    int maxEnergy() const;

    void setMaxEnergy(int value);
    void resetEnergy();

    bool canSpendEnergy(int amount) const;
    void spendEnergy(int amount);
    void gainEnergy(int amount);

private:
    int energy_ = 3;
    int maxEnergy_ = 3;
};
