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

}

EffectSystem::EffectSystem(
    const EffectResolver& effectResolver,
    const Targeting& targeting,
    const DamageSystem& damageSystem,
    const BlockSystem& blockSystem,
    const EnergySystem& energySystem,
    const DrawSystem& drawSystem,
    const StatusSystem& statusSystem,
    const DroneSystem& droneSystem,
    const GameEventBus* eventBus
)
    : effectResolver_(effectResolver),
      targeting_(targeting),
      damageSystem_(damageSystem),
      blockSystem_(blockSystem),
      energySystem_(energySystem),
      drawSystem_(drawSystem),
      statusSystem_(statusSystem),
      droneSystem_(droneSystem),
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

            droneSystem_.summonDrone(state, *effect.statusId, context.source, context.random);
            return;

        case EffectType::UseDrone:
            droneSystem_.useOldestDrone(state, context.random);
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
