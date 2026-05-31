#pragma once

#include "entities/EntityId.hpp"
#include "entities/EntityType.hpp"
#include "statuses/StatusDatabase.hpp"

#include <optional>
#include <string>

class CombatState;
class GameEventBus;

class StatusSystem {
public:
    explicit StatusSystem(const StatusDatabase& statusDatabase, const GameEventBus* eventBus = nullptr);

    void applyStatus(
        CombatState& state,
        EntityId target,
        const std::string& statusId,
        int amount,
        std::optional<EntityId> source = std::nullopt
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

    void emitPoisonDamageEvents(
        CombatState& state,
        EntityId owner,
        std::optional<EntityId> source,
        int hpDamage,
        bool killed
    ) const;

private:
    const StatusDatabase& statusDatabase_;
    const GameEventBus* eventBus_ = nullptr;
};
