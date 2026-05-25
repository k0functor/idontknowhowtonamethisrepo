#include "EffectSystem.hpp"

#include "combat/CombatState.hpp"

#include <stdexcept>

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
            energySystem_.gain(state, resolvedValue.actual);
            state.log.add("Gain energy: " + std::to_string(resolvedValue.actual));
            return;

        case EffectType::DiscardCards:
        case EffectType::GainStress:
        case EffectType::LoseEnergy:
        case EffectType::LoseStress:
        case EffectType::LoseHp:
            state.log.add("Effect not implemented yet: " + toString(effect.type));
            return;
    }

    throw std::runtime_error("Unknown effect type");
}
