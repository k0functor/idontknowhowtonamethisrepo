#pragma once

#include "combat/CombatLog.hpp"
#include "entities/EntityId.hpp"
#include "entities/EntityType.hpp"
#include "statuses/StatusDatabase.hpp"

#include <optional>
#include <string>

class CombatEntity;
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

    void onTurnEndedForEntity(
        CombatState& state,
        EntityId owner
    ) const;

private:
    void removeExclusiveGroupStatuses(
        CombatEntity& entity,
        const StatusDefinition& incomingDefinition
    ) const;

    void processEndTurnStatus(
        CombatState& state,
        EntityId owner,
        const std::string& statusId,
        int amount
    ) const;

    void applyTriggeredEffect(
        CombatState& state,
        EntityId owner,
        const std::string& statusId,
        int stacks,
        const StatusTriggerDefinition& trigger
    ) const;

    void applyTriggeredDamage(
        CombatState& state,
        EntityId owner,
        const std::string& statusId,
        int amount,
        StatusTriggerLogType logType
    ) const;

    void emitDamageOverTimeEvents(
        CombatState& state,
        EntityId owner,
        const std::string& statusId,
        std::optional<EntityId> source,
        int hpDamage,
        bool killed
    ) const;

private:
    const StatusDatabase& statusDatabase_;
    const GameEventBus* eventBus_ = nullptr;
};
