#include "RelicSystem.hpp"

#include "combat/CombatState.hpp"
#include "effects/EffectTarget.hpp"
#include <optional>
#include <stdexcept>


RelicSystem::RelicSystem(const RelicDatabase& database)
    : database_(database) {}

void RelicSystem::setRelics(const std::vector<std::string>& relicIds) {
    inventory_.setFromIds(relicIds);
}

void RelicSystem::setRelics(const RunState& run) {
    inventory_.setFromRun(run);
}

void RelicSystem::clear() {
    inventory_.clear();
}

const RelicInventory& RelicSystem::inventory() const {
    return inventory_;
}

RelicInventory& RelicSystem::inventory() {
    return inventory_;
}

void RelicSystem::startCombat() {
    inventory_.resetCombatState();
}

void RelicSystem::handleEvent(
    CombatState& state,
    const GameEvent& event,
    const EffectSystem& effectSystem,
    Random& random
) {
    for (RelicInstance& instance : inventory_.all()) {
        if (!database_.contains(instance.id)) {
            continue;
        }

        const std::optional<EntityId> owner = ownerSource(state, instance);
        const RelicDefinition& relic = database_.get(instance.id);

        for (const RelicTriggerDefinition& trigger : relic.triggers) {
            if (!triggerMatches(state, trigger, event, instance, owner)) {
                continue;
            }

            if (trigger.effects.empty()) {
                continue;
            }

            EffectContext context;
            if (owner.has_value()) {
                context.source = *owner;
            } else {
                context.source = event.source.value_or(defaultPlayerSource(state));
            }
            context.explicitTarget = event.target;
            context.cardInstanceId = event.cardInstanceId.value_or(CardInstanceId{});
            context.cardDefinitionId = CardId("relic." + instance.id.value);
            context.diceCorruption = DiceCorruption{};
            context.random = &random;

            effectSystem.applyEffects(state, trigger.effects, context);

            ++instance.triggersThisCombat;
            ++instance.totalTriggers;

            state.log.add(CombatLogEntryType::RelicTriggered, {{"relic", instance.id.value}});
        }
    }
}

bool RelicSystem::triggerMatches(
    const CombatState& state,
    const RelicTriggerDefinition& trigger,
    const GameEvent& event,
    const RelicInstance& instance,
    const std::optional<EntityId> owner
) const {
    if (trigger.eventType != event.type) {
        return false;
    }

    if (trigger.oncePerCombat && instance.triggersThisCombat > 0) {
        return false;
    }

    if (owner.has_value() && event.type != GameEventType::CombatStarted && event.source.has_value() && state.isPlayer(*event.source) && *event.source != *owner) {
        return false;
    }

    if (trigger.everyNTurns > 0) {
        if (event.turn <= 0) {
            return false;
        }

        if (event.turn % trigger.everyNTurns != 0) {
            return false;
        }
    }

    if (trigger.statusId.has_value() && event.statusId != *trigger.statusId) {
        return false;
    }

    if (trigger.cardType.has_value() && (!event.cardType.has_value() || event.cardType != *trigger.cardType)) {
        return false;
    }

    if (trigger.breakdownType.has_value() && event.breakdownType != *trigger.breakdownType) {
        return false;
    }

    if (trigger.minimumBreakdownSeverity > 0 && event.breakdownSeverity < trigger.minimumBreakdownSeverity) {
        return false;
    }

    if (trigger.minimumAmount > 0 && event.amount < trigger.minimumAmount) {
        return false;
    }

    if (trigger.sourceSide == "player") {
        return event.source.has_value() && state.isPlayer(*event.source);
    }

    if (trigger.sourceSide == "enemy") {
        return event.source.has_value() && state.isEnemy(*event.source);
    }

    return true;
}

std::optional<EntityId> RelicSystem::ownerSource(const CombatState& state, const RelicInstance& instance) const {
    if (instance.ownerActorDefinitionId.empty()) {
        return std::nullopt;
    }

    for (const CombatEntity& player : state.players) {
        if (player.definitionId == instance.ownerActorDefinitionId) {
            return player.id;
        }
    }

    return std::nullopt;
}

EntityId RelicSystem::defaultPlayerSource(const CombatState& state) const {
    const std::vector<EntityId> alivePlayers = state.alivePlayerIds();

    if (!alivePlayers.empty()) {
        return alivePlayers.front();
    }

    if (!state.players.empty()) {
        return state.players.front().id;
    }

    throw std::runtime_error("Relic effect requires a player source, but combat has no player"); // NOL10N: developer combat state diagnostic
}
