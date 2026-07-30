#include "DroneSystem.hpp"

#include <algorithm>
#include <cstddef>
#include <stdexcept>
#include <string>
#include <vector>

DroneSystem::DroneSystem(
    const DroneDatabase& drones,
    const EffectResolver& effectResolver,
    const Targeting& targeting,
    const DamageSystem& damageSystem,
    const BlockSystem& blockSystem,
    const EnergySystem& energySystem,
    const DrawSystem& drawSystem,
    const StatusSystem& statusSystem,
    const GameEventBus* eventBus
)
    : drones_(drones),
      effectResolver_(effectResolver),
      targeting_(targeting),
      damageSystem_(damageSystem),
      blockSystem_(blockSystem),
      energySystem_(energySystem),
      drawSystem_(drawSystem),
      statusSystem_(statusSystem),
      eventBus_(eventBus) {}

bool DroneSystem::isKnownDrone(const std::string& droneId) const {
    return drones_.contains(DroneId(droneId));
}

void DroneSystem::summonDrone(
    CombatState& state,
    const std::string& droneId,
    const EntityId owner,
    Random* /*random*/
) const {
    if (droneId.empty()) {
        return;
    }

    if (!drones_.contains(DroneId(droneId))) {
        throw std::runtime_error("Cannot summon unknown drone: '" + droneId + "'");
    }

    if (state.maxDroneSlots == 0) {
        return;
    }

    if (state.droneSlots.size() >= state.maxDroneSlots && !state.droneSlots.empty()) {
        state.droneSlots.erase(state.droneSlots.begin());
    }

    DroneSlot slot;
    slot.droneId = droneId;
    slot.owner = validOwnerOrFallback(state, owner);
    state.droneSlots.push_back(slot);
    state.log.add(CombatLogEntryType::DroneSummoned, {{"drone", droneId}});
}

void DroneSystem::useOldestDrone(CombatState& state, Random* random) const {
    if (state.droneSlots.empty()) {
        state.log.add(CombatLogEntryType::NoDrone);
        return;
    }

    for (std::size_t index = 0; index < state.droneSlots.size(); ++index) {
        const DroneSlot& slot = state.droneSlots[index];
        const DroneDefinition& definition = drones_.get(DroneId(slot.droneId));
        if (!definition.activeAction.has_value()) {
            continue;
        }

        if (activateDroneAt(state, index, random)) {
            return;
        }
    }

    state.log.add(CombatLogEntryType::DroneNoReadyAction);
}

bool DroneSystem::activateDroneAt(
    CombatState& state,
    const std::size_t index,
    Random* random
) const {
    if (index >= state.droneSlots.size()) {
        state.log.add(CombatLogEntryType::NoDrone);
        return false;
    }

    const DroneSlot slot = state.droneSlots[index];
    const DroneDefinition& definition = drones_.get(DroneId(slot.droneId));
    if (!definition.activeAction.has_value()) {
        state.log.add(CombatLogEntryType::DroneNoActiveAction, {{"drone", slot.droneId}});
        return false;
    }

    applyDroneAction(state, slot, *definition.activeAction, random, DroneActionKind::Active);
    state.droneSlots.erase(state.droneSlots.begin() + static_cast<std::ptrdiff_t>(index));
    state.log.add(CombatLogEntryType::DroneConsumed, {{"drone", slot.droneId}});
    return true;
}


void DroneSystem::processEndOfPlayerTurn(CombatState& state, Random& random) const {
    for (const DroneSlot& slot : state.droneSlots) {
        const DroneDefinition& definition = drones_.get(DroneId(slot.droneId));
        if (definition.passiveAction.has_value()) {
            applyDroneAction(state, slot, *definition.passiveAction, &random, DroneActionKind::Passive);
        }
    }
}

EntityId DroneSystem::validOwnerOrFallback(const CombatState& state, const EntityId owner) const {
    if (state.hasEntity(owner) && state.entity(owner).isAlive()) {
        return owner;
    }

    const std::vector<EntityId> alivePlayers = state.alivePlayerIds();
    if (!alivePlayers.empty()) {
        return alivePlayers.front();
    }

    if (!state.players.empty()) {
        return state.players.front().id;
    }

    return EntityId{};
}

std::optional<EntityId> DroneSystem::targetForEffect(
    const CombatState& state,
    const EffectTarget target,
    const EntityId owner
) const {
    switch (target) {
        case EffectTarget::Self:
            return validOwnerOrFallback(state, owner);

        case EffectTarget::SingleEnemy: {
            const std::vector<EntityId> enemies = state.aliveEnemyIds();
            if (enemies.empty()) {
                return std::nullopt;
            }
            return enemies.front();
        }

        case EffectTarget::Ally: {
            const std::vector<EntityId> allies = state.alivePlayerIds();
            if (allies.empty()) {
                return std::nullopt;
            }
            return allies.front();
        }

        case EffectTarget::AllEnemies:
        case EffectTarget::RandomEnemy:
        case EffectTarget::AllAllies:
        case EffectTarget::RandomAlly:
            return std::nullopt;
    }

    return std::nullopt;
}

void DroneSystem::applyDroneAction(
    CombatState& state,
    const DroneSlot& slot,
    const DroneActionDefinition& action,
    Random* random,
    const DroneActionKind /*kind*/
) const {
    const EntityId owner = validOwnerOrFallback(state, slot.owner);

    EffectContext baseContext;
    baseContext.source = owner;
    baseContext.cardDefinitionId = CardId(slot.droneId);
    baseContext.random = random;

    for (const EffectDefinition& effect : action.effects) {
        baseContext.explicitTarget = targetForEffect(state, effect.target, owner);

        if ((effect.target == EffectTarget::SingleEnemy || effect.target == EffectTarget::Ally) &&
            !baseContext.explicitTarget.has_value()) {
            continue;
        }

        applyDroneEffect(state, effect, baseContext);
    }

    if (!action.logTextId.empty() || !action.fallbackLog.empty()) {
        state.log.add(
            CombatLogEntryType::DroneAction,
            {{"action_text_id", action.logTextId}, {"action", action.fallbackLog}}
        );
    }
}

void DroneSystem::applyDroneEffect(
    CombatState& state,
    const EffectDefinition& effect,
    const EffectContext& baseContext
) const {
    const ResolvedEffectValue resolvedValue = effectResolver_.resolveForApply(
        effect.value,
        baseContext
    );

    const std::vector<EntityId> targets = targeting_.resolveTargets(
        state,
        effect.target,
        baseContext
    );

    switch (effect.type) {
        case EffectType::Damage:
            for (const EntityId target : targets) {
                int damage = resolvedValue.actual;
                if (state.hasEntity(target)) {
                    damage += std::max(0, state.entity(target).statuses.stacks("drone_mark"));
                }

                damageSystem_.dealDamage(
                    state,
                    baseContext.source,
                    target,
                    damage,
                    baseContext.cardDefinitionId,
                    baseContext.diceCorruption,
                    false
                );
            }
            return;

        case EffectType::Block:
            for (const EntityId target : targets) {
                blockSystem_.gainBlock(
                    state,
                    baseContext.source,
                    target,
                    resolvedValue.actual,
                    baseContext.cardDefinitionId,
                    baseContext.diceCorruption,
                    false
                );
            }
            return;

        case EffectType::Heal:
            for (const EntityId target : targets) {
                state.entity(target).health.heal(resolvedValue.actual);
                state.log.add(CombatLogEntryType::Heal, {{"amount", std::to_string(resolvedValue.actual)}});
            }
            return;

        case EffectType::ApplyStatus:
            if (!effect.statusId.has_value()) {
                throw std::runtime_error("drone apply_status effect requires status id");
            }

            for (const EntityId target : targets) {
                statusSystem_.applyStatus(
                    state,
                    target,
                    *effect.statusId,
                    resolvedValue.actual,
                    baseContext.source
                );

                if (eventBus_ != nullptr) {
                    GameEvent event;
                    event.type = GameEventType::StatusApplied;
                    event.source = baseContext.source;
                    event.target = target;
                    event.cardDefinitionId = baseContext.cardDefinitionId;
                    event.statusId = *effect.statusId;
                    event.amount = resolvedValue.actual;
                    event.turn = state.turn;
                    eventBus_->emit(event);
                }
            }
            return;

        case EffectType::GainEnergy:
            energySystem_.gain(state, baseContext.source, resolvedValue.actual);
            return;

        case EffectType::DrawCards:
            if (baseContext.random == nullptr) {
                throw std::runtime_error("drone draw_cards effect requires Random");
            }
            drawSystem_.drawCards(
                state.deck,
                state.hand,
                static_cast<std::size_t>(std::max(0, resolvedValue.actual)),
                *baseContext.random
            );
            state.log.add(CombatLogEntryType::DrawCards, {{"amount", std::to_string(resolvedValue.actual)}});
            return;

        case EffectType::DiscardCards:
        case EffectType::RecoverCards:
        case EffectType::GainStress:
        case EffectType::SpendStressDamage:
        case EffectType::SpendStressBlock:
        case EffectType::SpendStressEnergy:
        case EffectType::SpendStressDraw:
        case EffectType::LoseEnergy:
        case EffectType::LoseStress:
        case EffectType::LoseHp:
        case EffectType::EnterStance:
        case EffectType::SummonDrone:
        case EffectType::UseDrone:
        case EffectType::PrimeStressBreakdown:
            throw std::runtime_error(
                "Unsupported effect type in drone definition: '" + toString(effect.type) + "'"
            );
    }

    throw std::runtime_error("Unknown effect type in drone definition");
}
