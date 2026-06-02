#include "EffectSystem.hpp"

#include "combat/CombatState.hpp"
#include "run/StressRules.hpp"

#include <algorithm>
#include <cstddef>
#include <limits>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {
constexpr const char* monkActorId = "monk";
constexpr const char* stanceFlame = "stance_flame";
constexpr const char* stanceAsh = "stance_ash";
constexpr const char* stanceSmoke = "stance_smoke";
constexpr int monkStanceShiftDraw = 2;
constexpr int monkStanceShiftEnergy = 1;

bool isStanceStatus(const std::string& statusId) {
    return statusId == stanceFlame || statusId == stanceAsh || statusId == stanceSmoke;
}

std::optional<std::string> activeStance(const CombatEntity& entity) {
    if (entity.statuses.has(stanceFlame)) {
        return std::string(stanceFlame);
    }

    if (entity.statuses.has(stanceAsh)) {
        return std::string(stanceAsh);
    }

    if (entity.statuses.has(stanceSmoke)) {
        return std::string(stanceSmoke);
    }

    return std::nullopt;
}

bool shouldTriggerMonkStanceShiftReward(
    const CombatEntity& entity,
    const std::optional<std::string>& previousStance,
    const std::string& nextStance
) {
    return entity.type == EntityType::Player &&
           entity.definitionId == monkActorId &&
           previousStance.has_value() &&
           *previousStance != nextStance &&
           isStanceStatus(nextStance);
}

std::size_t chooseDiscardIndex(const std::vector<CardInstance>& cards, const EffectContext& context) {
    std::vector<std::size_t> candidates;
    candidates.reserve(cards.size());

    for (std::size_t index = 0; index < cards.size(); ++index) {
        if (context.cardInstanceId.value != 0 && cards[index].instanceId == context.cardInstanceId) {
            continue;
        }
        candidates.push_back(index);
    }

    if (candidates.empty()) {
        return std::numeric_limits<std::size_t>::max();
    }

    if (context.random == nullptr) {
        return candidates.front();
    }

    const int rolledIndex = context.random->rangeInclusive(0, static_cast<int>(candidates.size() - 1));
    return candidates[static_cast<std::size_t>(rolledIndex)];
}

int discardRandomCardsFromHand(CombatState& state, const int amount, const EffectContext& context) {
    if (amount <= 0) {
        return 0;
    }

    int discardedCount = 0;

    while (discardedCount < amount) {
        std::vector<CardInstance>& cards = state.hand.cards();
        const std::size_t discardIndex = chooseDiscardIndex(cards, context);
        if (discardIndex == std::numeric_limits<std::size_t>::max()) {
            break;
        }

        CardInstance discarded = std::move(cards[discardIndex]);
        cards.erase(cards.begin() + static_cast<std::ptrdiff_t>(discardIndex));
        state.deck.discardPile.addTop(std::move(discarded));
        ++discardedCount;
    }

    return discardedCount;
}

void logStressResolveOutcome(CombatState& state, const CombatEntity& entity, const StressRules::StressAdjustmentResult& result) {
    if (!result.resolveCheckTriggered) {
        return;
    }

    switch (result.resolveOutcome) {
        case StressRules::ResolveOutcome::Resolve:
            state.log.add(
                CombatLogEntryType::StressResolve,
                {{"actor", entity.definitionId}, {"actor_text_id", entity.nameTextId.value}}
            );
            return;

        case StressRules::ResolveOutcome::Breakdown:
            state.log.add(
                CombatLogEntryType::StressBreakdown,
                {{"actor", entity.definitionId}, {"actor_text_id", entity.nameTextId.value}}
            );
            return;

        case StressRules::ResolveOutcome::None:
            return;
    }
}

void adjustStress(CombatState& state, const EntityId target, const int delta, Random* random) {
    CombatEntity& entity = state.entity(target);
    const StressRules::StressAdjustmentResult result = StressRules::applyDelta(entity, delta, random);

    if (result.applied > 0) {
        state.log.add(CombatLogEntryType::GainStress, {{"amount", std::to_string(result.applied)}});
    } else if (result.applied < 0) {
        state.log.add(CombatLogEntryType::LoseStress, {{"amount", std::to_string(-result.applied)}});
    }

    if (entity.type == EntityType::Player) {
        logStressResolveOutcome(state, entity, result);

        if (result.collapsed) {
            entity.health.setCurrent(0);
            state.log.add(
                CombatLogEntryType::StressCollapse,
                {{"actor", entity.definitionId}, {"actor_text_id", entity.nameTextId.value}}
            );
        }
    }
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
    for (int repeatIndex = 0; repeatIndex < effect.repeatCount; ++repeatIndex) {
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
                continue;

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
                continue;

            case EffectType::ApplyStatus:
                if (!effect.statusId.has_value()) {
                    throw std::runtime_error("apply_status effect requires status id");
                }

                for (const EntityId target : targets) {
                    statusSystem_.applyStatus(
                        state,
                        target,
                        *effect.statusId,
                        resolvedValue.actual,
                        context.source
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
                continue;

            case EffectType::EnterStance:
                if (!effect.statusId.has_value()) {
                    throw std::runtime_error("enter_stance effect requires status id");
                }

                for (const EntityId target : targets) {
                    CombatEntity& entity = state.entity(target);
                    const std::optional<std::string> previousStance = activeStance(entity);

                    statusSystem_.applyStatus(state, target, *effect.statusId, 1, context.source);

                    if (shouldTriggerMonkStanceShiftReward(entity, previousStance, *effect.statusId)) {
                        std::size_t drawn = 0;
                        if (context.random != nullptr) {
                            drawn = drawSystem_.drawCards(
                                state.deck,
                                state.hand,
                                static_cast<std::size_t>(monkStanceShiftDraw),
                                *context.random
                            );
                        }

                        energySystem_.gain(state, target, monkStanceShiftEnergy);
                        state.log.add(
                            CombatLogEntryType::MonkStanceShiftReward,
                            {
                                {"draw", std::to_string(drawn)},
                                {"energy", std::to_string(monkStanceShiftEnergy)}
                            }
                        );
                    }
                }
                continue;

            case EffectType::SummonDrone:
                if (!effect.statusId.has_value()) {
                    throw std::runtime_error("summon_drone effect requires drone id in status field");
                }

                droneSystem_.summonDrone(state, *effect.statusId, context.source, context.random);
                continue;

            case EffectType::UseDrone:
                droneSystem_.useOldestDrone(state, context.random);
                continue;

            case EffectType::Heal:
                for (const EntityId target : targets) {
                    const int healed = state.entity(target).health.heal(resolvedValue.actual);
                    state.log.add(CombatLogEntryType::Heal, {{"amount", std::to_string(healed)}});

                    if (eventBus_ != nullptr && healed > 0) {
                        GameEvent event;
                        event.type = GameEventType::Healed;
                        event.source = context.source;
                        event.target = target;
                        event.cardInstanceId = context.cardInstanceId;
                        event.cardDefinitionId = context.cardDefinitionId;
                        event.effectType = EffectType::Heal;
                        event.amount = healed;
                        event.turn = state.turn;
                        eventBus_->emit(event);
                    }
                }
                continue;

            case EffectType::DrawCards:
                drawSystem_.drawCards(
                    state.deck,
                    state.hand,
                    static_cast<std::size_t>(resolvedValue.actual),
                    *context.random
                );
                state.log.add(CombatLogEntryType::DrawCards, {{"amount", std::to_string(resolvedValue.actual)}});
                continue;

            case EffectType::DiscardCards: {
                const bool targetsPlayer = std::any_of(targets.begin(), targets.end(), [&state](const EntityId target) {
                    return state.isPlayer(target);
                });
                const int discarded = targetsPlayer
                    ? discardRandomCardsFromHand(state, resolvedValue.actual, context)
                    : 0;
                state.log.add(CombatLogEntryType::DiscardCards, {{"amount", std::to_string(discarded)}});
                continue;
            }

            case EffectType::GainEnergy:
                energySystem_.gain(state, context.source, resolvedValue.actual);
                state.log.add(CombatLogEntryType::GainEnergy, {{"amount", std::to_string(resolvedValue.actual)}});
                continue;

            case EffectType::LoseEnergy: {
                int lost = 0;
                for (const EntityId target : targets) {
                    if (!state.isPlayer(target)) {
                        continue;
                    }
                    lost += energySystem_.lose(state, target, resolvedValue.actual);
                }
                state.log.add(CombatLogEntryType::LoseEnergy, {{"amount", std::to_string(lost)}});
                continue;
            }

            case EffectType::LoseHp:
                for (const EntityId target : targets) {
                    const int hpDamage = state.entity(target).health.takeDamage(resolvedValue.actual);
                    state.log.add(CombatLogEntryType::LoseHp, {{"amount", std::to_string(hpDamage)}});
                }
                continue;

            case EffectType::GainStress:
                for (const EntityId target : targets) {
                    adjustStress(state, target, resolvedValue.actual, context.random);
                }
                continue;

            case EffectType::LoseStress:
                for (const EntityId target : targets) {
                    adjustStress(state, target, -resolvedValue.actual, context.random);
                }
                continue;

        }

        throw std::runtime_error("Unknown effect type");
    }
}
