#include "StatusSystem.hpp"

#include "combat/CombatState.hpp"

#include <stdexcept>

namespace {
constexpr const char* poisonDamageEffect = "poison_damage";
}

StatusSystem::StatusSystem(const StatusDatabase& statusDatabase)
    : statusDatabase_(statusDatabase) {}

void StatusSystem::applyStatus(
    CombatState& state,
    const EntityId target,
    const std::string& statusId,
    const int amount
) const {
    if (amount <= 0) {
        return;
    }

    if (!statusDatabase_.contains(StatusId(statusId))) {
        throw std::runtime_error("Cannot apply unknown status: '" + statusId + "'");
    }

    CombatEntity& targetEntity = state.entity(target);
    targetEntity.statuses.add(statusId, amount);

    state.log.add(
        "Status: " + statusId +
        " +" + std::to_string(amount) +
        " on entity " + std::to_string(target.value)
    );
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

void StatusSystem::processEndTurnStatus(
    CombatState& state,
    const EntityId owner,
    const std::string& statusId,
    const int amount
) const {
    if (amount <= 0 || !statusDatabase_.contains(StatusId(statusId))) {
        return;
    }

    const StatusDefinition& definition = statusDatabase_.get(StatusId(statusId));
    CombatEntity& entity = state.entity(owner);

    bool alreadyDecreased = false;

    if (definition.endTurnEffect == poisonDamageEffect) {
        applyPoisonDamage(state, owner, amount);

        if (definition.decreaseAfterTrigger) {
            entity.statuses.add(statusId, -1);
            alreadyDecreased = true;
        }
    }

    if (definition.durationRule == StatusDurationRule::DecreaseEndOfOwnerTurn && !alreadyDecreased) {
        entity.statuses.add(statusId, -1);
    }
}

void StatusSystem::applyPoisonDamage(
    CombatState& state,
    const EntityId owner,
    const int amount
) const {
    if (amount <= 0) {
        return;
    }

    CombatEntity& entity = state.entity(owner);
    const int hpDamage = entity.health.takeDamage(amount);

    state.log.add(
        "Poison damage: entity " + std::to_string(owner.value) +
        " takes " + std::to_string(hpDamage)
    );
}
