#include "StatusSystem.hpp"

#include "combat/CombatState.hpp"
#include "game/GameEvent.hpp"
#include "game/GameEventBus.hpp"

#include <algorithm>
#include <stdexcept>

namespace {
constexpr const char* poisonDamageEffect = "poison_damage";
constexpr const char* poisonStatus = "poison";
constexpr const char* stanceFlame = "stance_flame";
constexpr const char* stanceAsh = "stance_ash";
constexpr const char* stanceSmoke = "stance_smoke";

bool isStanceStatus(const std::string& statusId) {
    return statusId == stanceFlame || statusId == stanceAsh || statusId == stanceSmoke;
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

    if (!statusDatabase_.contains(StatusId(statusId))) {
        throw std::runtime_error("Cannot apply unknown status: '" + statusId + "'");
    }

    CombatEntity& targetEntity = state.entity(target);

    if (isStanceStatus(statusId)) {
        targetEntity.statuses.remove(stanceFlame);
        targetEntity.statuses.remove(stanceAsh);
        targetEntity.statuses.remove(stanceSmoke);
        targetEntity.statuses.set(statusId, 1, source);
    } else {
        targetEntity.statuses.add(statusId, amount, source);
    }

    state.log.add(
        CombatLogEntryType::StatusApplied,
        {
            {"status", statusId},
            {"amount", std::to_string(amount)},
            {"target", targetEntity.definitionId.empty() ? std::to_string(target.value) : targetEntity.definitionId},
            {"target_text_id", targetEntity.nameTextId.value}
        }
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
    const std::optional<EntityId> source = entity.statuses.source(poisonStatus);
    const int hpDamage = entity.health.takeDamage(amount);
    const bool killed = entity.health.isDead();

    state.log.add(
        CombatLogEntryType::PoisonDamage,
        {
            {"target", entity.definitionId.empty() ? std::to_string(owner.value) : entity.definitionId},
            {"target_text_id", entity.nameTextId.value},
            {"amount", std::to_string(hpDamage)},
            {"stacks", std::to_string(amount)},
            {"remaining", std::to_string(std::max(0, amount - 1))}
        }
    );

    emitPoisonDamageEvents(state, owner, source, hpDamage, killed);
}

void StatusSystem::emitPoisonDamageEvents(
    CombatState& state,
    const EntityId owner,
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
    dealt.cardDefinitionId = CardId("status.poison");
    dealt.effectType = EffectType::Damage;
    dealt.statusId = poisonStatus;
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
