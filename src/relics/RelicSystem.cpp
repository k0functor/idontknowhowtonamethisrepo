#include "RelicSystem.hpp"

#include "combat/CombatState.hpp"
#include "effects/EffectTarget.hpp"
#include "localization/LocalizationManager.hpp"
#include "localization/TextId.hpp"

#include <iomanip>
#include <sstream>
#include <stdexcept>

namespace {
std::string compactDouble(const double value) {
    std::ostringstream out;
    out << std::fixed << std::setprecision(2) << value;

    std::string result = out.str();
    while (!result.empty() && result.back() == '0') {
        result.pop_back();
    }
    if (!result.empty() && result.back() == '.') {
        result.pop_back();
    }

    return result.empty() ? "0" : result;
}
}

RelicSystem::RelicSystem(const RelicDatabase& database, const LocalizationManager& localization)
    : database_(database),
      localization_(localization) {}

void RelicSystem::setRelics(const std::vector<std::string>& relicIds) {
    inventory_.setFromIds(relicIds);
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

void RelicSystem::collectModifiers(
    const CombatState& state,
    const ModifierContext& context,
    std::vector<ValueModifier>& output
) const {
    if (!state.hasEntity(context.source)) {
        return;
    }

    if (!state.isPlayer(context.source)) {
        return;
    }

    for (const RelicInstance& instance : inventory_.all()) {
        if (!database_.contains(instance.id)) {
            continue;
        }

        const RelicDefinition& relic = database_.get(instance.id);

        for (const RelicModifierDefinition& modifier : relic.modifiers) {
            if (modifier.playerOnly && !state.isPlayer(context.source)) {
                continue;
            }

            switch (modifier.type) {
                case RelicModifierType::OutgoingDamageAdd:
                    if (context.effectType == EffectType::Damage) {
                        output.push_back({
                            instance.id.value,
                            localization_.format(TextId("modifier.relic.outgoing_damage_add"), {{"amount", std::to_string(modifier.amount)}}),
                            ModifierOperation::Add,
                            modifier.amount,
                            1.0,
                            modifier.priority
                        });
                    }
                    break;

                case RelicModifierType::OutgoingDamageMultiply:
                    if (context.effectType == EffectType::Damage) {
                        output.push_back({
                            instance.id.value,
                            localization_.format(TextId("modifier.relic.outgoing_damage_multiply"), {{"multiplier", compactDouble(modifier.multiplier)}}),
                            ModifierOperation::Multiply,
                            0,
                            modifier.multiplier,
                            modifier.priority
                        });
                    }
                    break;

                case RelicModifierType::BlockAdd:
                    if (context.effectType == EffectType::Block) {
                        output.push_back({
                            instance.id.value,
                            localization_.format(TextId("modifier.relic.block_add"), {{"amount", std::to_string(modifier.amount)}}),
                            ModifierOperation::Add,
                            modifier.amount,
                            1.0,
                            modifier.priority
                        });
                    }
                    break;

                case RelicModifierType::GoldRewardMultiply:
                    // Reward modifiers are handled by RewardGenerator, not combat ModifierSystem.
                    break;
            }
        }
    }
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

        const RelicDefinition& relic = database_.get(instance.id);

        for (const RelicTriggerDefinition& trigger : relic.triggers) {
            if (!triggerMatches(state, trigger, event, instance)) {
                continue;
            }

            if (trigger.effects.empty()) {
                continue;
            }

            EffectContext context;
            context.source = event.source.value_or(defaultPlayerSource(state));
            context.explicitTarget = event.target;
            context.cardInstanceId = event.cardInstanceId.value_or(CardInstanceId{});
            context.cardDefinitionId = CardId("relic." + instance.id.value);
            context.diceCorruption = DiceCorruption{};
            context.random = &random;

            // For self-targeted relic effects, the source is enough. For explicit target
            // effects, event.target remains available through context.explicitTarget.
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
    const RelicInstance& instance
) const {
    if (trigger.eventType != event.type) {
        return false;
    }

    if (trigger.oncePerCombat && instance.triggersThisCombat > 0) {
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
