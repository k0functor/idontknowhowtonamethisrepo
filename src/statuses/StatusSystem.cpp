#include "StatusSystem.hpp"

#include "combat/CombatState.hpp"
#include "game/GameEvent.hpp"
#include "game/GameEventBus.hpp"

#include <algorithm>
#include <optional>
#include <stdexcept>

namespace {
std::optional<CombatLogEntryType> combatLogType(const StatusTriggerLogType type) {
    switch (type) {
        case StatusTriggerLogType::None:
            return std::nullopt;
        case StatusTriggerLogType::PoisonDamage:
            return CombatLogEntryType::PoisonDamage;
        case StatusTriggerLogType::BurnDamage:
            return CombatLogEntryType::BurnDamage;
    }

    return std::nullopt;
}
}

StatusSystem::StatusSystem(const StatusDatabase& statusDatabase, const GameEventBus* eventBus)
    : statusDatabase_(statusDatabase),
      eventBus_(eventBus) {}

void StatusSystem::applyStatus(
    CombatState& state,
    const EntityId target,
    const std::string& statusId,
    const int amount,
    const std::optional<EntityId> source
) const {
    if (amount <= 0) {
        return;
    }

    const StatusId id(statusId);
    if (!statusDatabase_.contains(id)) {
        throw std::runtime_error("Cannot apply unknown status: '" + statusId + "'");
    }

    state.rememberStatusSeen(statusId);

    CombatEntity& targetEntity = state.entity(target);
    const StatusDefinition& definition = statusDatabase_.get(id);

    if (!definition.exclusiveGroup.empty()) {
        removeExclusiveGroupStatuses(targetEntity, definition);
        targetEntity.statuses.set(statusId, amount, source);
    } else {
        targetEntity.statuses.add(statusId, amount, source);
    }

    CombatLogEntry::Variables logVariables{
        {"status", statusId},
        {"amount", std::to_string(amount)},
        {"target", targetEntity.definitionId.empty() ? std::to_string(target.value) : targetEntity.definitionId},
        {"target_text_id", targetEntity.nameTextId.value}
    };
    if (source.has_value() && state.hasEntity(*source)) {
        const CombatEntity& sourceEntity = state.entity(*source);
        logVariables["source"] = sourceEntity.definitionId.empty()
            ? std::to_string(source->value)
            : sourceEntity.definitionId;
        logVariables["source_text_id"] = sourceEntity.nameTextId.value;
    }
    state.log.add(CombatLogEntryType::StatusApplied, std::move(logVariables));
}

void StatusSystem::removeExclusiveGroupStatuses(
    CombatEntity& entity,
    const StatusDefinition& incomingDefinition
) const {
    if (incomingDefinition.exclusiveGroup.empty()) {
        return;
    }

    for (const auto& [activeStatusId, stacks] : entity.statuses.all()) {
        if (stacks <= 0 || activeStatusId == incomingDefinition.id.value) {
            continue;
        }

        const StatusId activeId(activeStatusId);
        if (!statusDatabase_.contains(activeId)) {
            continue;
        }

        const StatusDefinition& activeDefinition = statusDatabase_.get(activeId);
        if (activeDefinition.exclusiveGroup == incomingDefinition.exclusiveGroup) {
            entity.statuses.remove(activeStatusId);
        }
    }
}

void StatusSystem::onTurnEndedForSide(
    CombatState& state,
    const EntityType ownerType
) const {
    std::vector<CombatEntity>* entities = nullptr;

    if (ownerType == EntityType::Player) {
        entities = &state.players;
    } else if (ownerType == EntityType::Enemy) {
        entities = &state.enemies;
    } else {
        return;
    }

    for (CombatEntity& entity : *entities) {
        if (!entity.isAlive()) {
            continue;
        }

        const std::vector<std::pair<std::string, int>> statuses = entity.statuses.all();
        for (const auto& [statusId, amount] : statuses) {
            processEndTurnStatus(state, entity.id, statusId, amount);
        }
    }
}

void StatusSystem::onTurnEndedForEntity(
    CombatState& state,
    const EntityId owner
) const {
    if (!state.hasEntity(owner)) {
        return;
    }

    CombatEntity& entity = state.entity(owner);
    if (!entity.isAlive()) {
        return;
    }

    const std::vector<std::pair<std::string, int>> statuses = entity.statuses.all();
    for (const auto& [statusId, amount] : statuses) {
        processEndTurnStatus(state, owner, statusId, amount);
    }
}

void StatusSystem::processEndTurnStatus(
    CombatState& state,
    const EntityId owner,
    const std::string& statusId,
    const int amount
) const {
    const StatusId id(statusId);
    if (amount <= 0 || !statusDatabase_.contains(id)) {
        return;
    }

    const StatusDefinition& definition = statusDatabase_.get(id);
    CombatEntity& entity = state.entity(owner);

    int stacksToRemove = 0;
    for (const StatusTriggerDefinition& trigger : definition.triggers) {
        if (trigger.event != StatusTriggerEvent::EndOwnerTurn) {
            continue;
        }

        applyTriggeredEffect(state, owner, statusId, amount, trigger);
        stacksToRemove += std::max(0, trigger.removeStacks);
    }

    if (definition.durationRule == StatusDurationRule::DecreaseEndOfOwnerTurn) {
        ++stacksToRemove;
    }

    if (stacksToRemove > 0) {
        entity.statuses.add(statusId, -stacksToRemove);
    }
}

void StatusSystem::applyTriggeredEffect(
    CombatState& state,
    const EntityId owner,
    const std::string& statusId,
    const int stacks,
    const StatusTriggerDefinition& trigger
) const {
    const int amount = std::max(0, trigger.flatValue + trigger.valuePerStack * stacks);
    if (amount <= 0 || !state.hasEntity(owner)) {
        return;
    }

    CombatEntity& entity = state.entity(owner);
    switch (trigger.effect) {
        case StatusTriggeredEffect::DamageHp:
            applyTriggeredDamage(state, owner, statusId, amount, trigger.logType);
            return;

        case StatusTriggeredEffect::Heal: {
            const int healed = entity.health.heal(amount);
            if (healed > 0) {
                state.log.add(
                    CombatLogEntryType::Heal,
                    {
                        {"target", entity.definitionId.empty() ? std::to_string(owner.value) : entity.definitionId},
                        {"target_text_id", entity.nameTextId.value},
                        {"amount", std::to_string(healed)},
                        {"status", statusId}
                    }
                );
            }
            return;
        }

        case StatusTriggeredEffect::GainBlock:
            entity.block += amount;
            state.log.add(
                CombatLogEntryType::BlockGained,
                {
                    {"target", entity.definitionId.empty() ? std::to_string(owner.value) : entity.definitionId},
                    {"target_text_id", entity.nameTextId.value},
                    {"amount", std::to_string(amount)},
                    {"status", statusId}
                }
            );
            return;
    }
}

void StatusSystem::applyTriggeredDamage(
    CombatState& state,
    const EntityId owner,
    const std::string& statusId,
    const int amount,
    const StatusTriggerLogType logType
) const {
    CombatEntity& entity = state.entity(owner);
    const std::optional<EntityId> source = entity.statuses.source(statusId);
    const int hpDamage = entity.health.takeDamage(amount);
    const bool killed = entity.health.isDead();

    if (const std::optional<CombatLogEntryType> type = combatLogType(logType); type.has_value()) {
        state.log.add(
            *type,
            {
                {"target", entity.definitionId.empty() ? std::to_string(owner.value) : entity.definitionId},
                {"target_text_id", entity.nameTextId.value},
                {"amount", std::to_string(hpDamage)},
                {"stacks", std::to_string(entity.statuses.stacks(statusId))},
                {"remaining", std::to_string(std::max(0, entity.statuses.stacks(statusId) - 1))}
            }
        );
    }

    emitDamageOverTimeEvents(state, owner, statusId, source, hpDamage, killed);
}

void StatusSystem::emitDamageOverTimeEvents(
    CombatState& state,
    const EntityId owner,
    const std::string& statusId,
    const std::optional<EntityId> source,
    const int hpDamage,
    const bool killed
) const {
    if (eventBus_ == nullptr || hpDamage <= 0) {
        return;
    }

    GameEvent dealt;
    dealt.type = GameEventType::DamageDealt;
    dealt.source = source;
    dealt.target = owner;
    dealt.cardDefinitionId = CardId("status." + statusId);
    dealt.effectType = EffectType::Damage;
    dealt.statusId = statusId;
    dealt.amount = hpDamage;
    dealt.turn = state.turn;

    if (source.has_value()) {
        eventBus_->emit(dealt);
    }

    GameEvent taken = dealt;
    taken.type = GameEventType::DamageTaken;
    eventBus_->emit(taken);

    if (killed && state.isEnemy(owner)) {
        GameEvent killedEvent = dealt;
        killedEvent.type = GameEventType::EnemyKilled;
        eventBus_->emit(killedEvent);
    }
}
