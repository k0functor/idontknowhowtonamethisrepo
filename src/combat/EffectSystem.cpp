#include "EffectSystem.hpp"

#include "combat/CombatState.hpp"

#include <algorithm>
#include <stdexcept>

namespace {
constexpr const char* stanceFlame = "stance_flame";
constexpr const char* stanceAsh = "stance_ash";
constexpr const char* stanceSmoke = "stance_smoke";

void clearStances(CombatEntity& entity) {
    entity.statuses.remove(stanceFlame);
    entity.statuses.remove(stanceAsh);
    entity.statuses.remove(stanceSmoke);
}

EntityId firstAlivePlayer(const CombatState& state) {
    const std::vector<EntityId> players = state.alivePlayerIds();
    if (!players.empty()) {
        return players.front();
    }

    if (!state.players.empty()) {
        return state.players.front().id;
    }

    return EntityId{};
}

void useDrone(CombatState& state, const std::string& droneType) {
    if (droneType == "drone_striker") {
        const std::vector<EntityId> enemies = state.aliveEnemyIds();
        if (!enemies.empty()) {
            CombatEntity& enemy = state.entity(enemies.front());
            const int damage = enemy.health.takeDamage(3);
            state.log.add("Striker drone deals " + std::to_string(damage) + " damage");
        }
        return;
    }

    if (droneType == "drone_guardian") {
        const EntityId owner = firstAlivePlayer(state);
        if (state.hasEntity(owner)) {
            state.entity(owner).block += 4;
            state.log.add("Guardian drone grants 4 block");
        }
        return;
    }

    if (droneType == "drone_bomber") {
        for (const EntityId enemyId : state.aliveEnemyIds()) {
            CombatEntity& enemy = state.entity(enemyId);
            enemy.health.takeDamage(8);
        }
        state.log.add("Bomber drone explodes for 8 damage to all enemies");
        return;
    }

    state.log.add("Unknown drone used: " + droneType);
}

void summonDrone(CombatState& state, const std::string& droneType) {
    if (droneType.empty()) {
        return;
    }

    if (state.droneSlots.size() >= state.maxDroneSlots && !state.droneSlots.empty()) {
        const std::string oldest = state.droneSlots.front().type;
        state.droneSlots.erase(state.droneSlots.begin());
        useDrone(state, oldest);
    }

    state.droneSlots.push_back(DroneSlot{droneType});
    state.log.add("Summoned drone: " + droneType);
}
}

EffectSystem::EffectSystem(
    const EffectResolver& effectResolver,
    const Targeting& targeting,
    const DamageSystem& damageSystem,
    const BlockSystem& blockSystem,
    const EnergySystem& energySystem,
    const DrawSystem& drawSystem,
    const StatusSystem& statusSystem,
    const GameEventBus* eventBus
)
    : effectResolver_(effectResolver),
      targeting_(targeting),
      damageSystem_(damageSystem),
      blockSystem_(blockSystem),
      energySystem_(energySystem),
      drawSystem_(drawSystem),
      statusSystem_(statusSystem),
      eventBus_(eventBus) {}

void EffectSystem::applyEffects(
    CombatState& state,
    const std::vector<EffectDefinition>& effects,
    const EffectContext& context
) const {
    for (const EffectDefinition& effect : effects) {
        applyEffect(state, effect, context);
    }
}

void EffectSystem::applyEffect(
    CombatState& state,
    const EffectDefinition& effect,
    const EffectContext& context
) const {
    const ResolvedEffectValue resolvedValue = effectResolver_.resolveForApply(
        effect.value,
        context
    );

    const std::vector<EntityId> targets = targeting_.resolveTargets(
        state,
        effect.target,
        context
    );

    switch (effect.type) {
        case EffectType::Damage:
            for (const EntityId target : targets) {
                damageSystem_.dealDamage(
                    state,
                    context.source,
                    target,
                    resolvedValue.actual,
                    context.cardDefinitionId,
                    context.diceCorruption
                );
            }
            return;

        case EffectType::Block:
            for (const EntityId target : targets) {
                blockSystem_.gainBlock(
                    state,
                    context.source,
                    target,
                    resolvedValue.actual,
                    context.cardDefinitionId,
                    context.diceCorruption
                );
            }
            return;

        case EffectType::ApplyStatus:
            if (!effect.statusId.has_value()) {
                throw std::runtime_error("apply_status effect requires status id");
            }

            for (const EntityId target : targets) {
                statusSystem_.applyStatus(
                    state,
                    target,
                    *effect.statusId,
                    resolvedValue.actual
                );

                if (eventBus_ != nullptr) {
                    GameEvent event;
                    event.type = GameEventType::StatusApplied;
                    event.source = context.source;
                    event.target = target;
                    event.cardInstanceId = context.cardInstanceId;
                    event.cardDefinitionId = context.cardDefinitionId;
                    event.statusId = *effect.statusId;
                    event.amount = resolvedValue.actual;
                    event.turn = state.turn;
                    eventBus_->emit(event);
                }
            }
            return;

        case EffectType::EnterStance:
            if (!effect.statusId.has_value()) {
                throw std::runtime_error("enter_stance effect requires status id");
            }

            for (const EntityId target : targets) {
                CombatEntity& entity = state.entity(target);
                clearStances(entity);
                statusSystem_.applyStatus(state, target, *effect.statusId, 1);
            }
            return;

        case EffectType::SummonDrone:
            if (!effect.statusId.has_value()) {
                throw std::runtime_error("summon_drone effect requires drone id in status field");
            }

            summonDrone(state, *effect.statusId);
            return;

        case EffectType::UseDrone:
            if (!state.droneSlots.empty()) {
                const std::string droneType = state.droneSlots.front().type;
                state.droneSlots.erase(state.droneSlots.begin());
                useDrone(state, droneType);
            } else {
                state.log.add("No drone to use");
            }
            return;

        case EffectType::Heal:
            for (const EntityId target : targets) {
                state.entity(target).health.heal(resolvedValue.actual);
                state.log.add("Heal: " + std::to_string(resolvedValue.actual));
            }
            return;

        case EffectType::DrawCards:
            drawSystem_.drawCards(
                state.deck,
                state.hand,
                static_cast<std::size_t>(resolvedValue.actual),
                *context.random
            );
            state.log.add("Draw cards: " + std::to_string(resolvedValue.actual));
            return;

        case EffectType::GainEnergy:
            energySystem_.gain(state, context.source, resolvedValue.actual);
            state.log.add("Gain energy: " + std::to_string(resolvedValue.actual));
            return;

        case EffectType::LoseHp:
            for (const EntityId target : targets) {
                const int hpDamage = state.entity(target).health.takeDamage(resolvedValue.actual);
                state.log.add("Lose HP: " + std::to_string(hpDamage));
            }
            return;

        case EffectType::DiscardCards:
        case EffectType::GainStress:
        case EffectType::LoseEnergy:
        case EffectType::LoseStress:
            state.log.add("Effect not implemented yet: " + toString(effect.type));
            return;
    }

    throw std::runtime_error("Unknown effect type");
}
