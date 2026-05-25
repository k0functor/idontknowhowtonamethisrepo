#pragma once

#include "entities/EntityId.hpp"
#include "entities/EntityType.hpp"
#include "statuses/StatusDatabase.hpp"

#include <string>

class CombatState;

class StatusSystem {
public:
    explicit StatusSystem(const StatusDatabase& statusDatabase);

    void applyStatus(
        CombatState& state,
        EntityId target,
        const std::string& statusId,
        int amount
    ) const;

    void onTurnEndedForSide(
        CombatState& state,
        EntityType ownerType
    ) const;

private:
    void processEndTurnStatus(
        CombatState& state,
        EntityId owner,
        const std::string& statusId,
        int amount
    ) const;

    void applyPoisonDamage(
        CombatState& state,
        EntityId owner,
        int amount
    ) const;

private:
    const StatusDatabase& statusDatabase_;
};
