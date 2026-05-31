#include "InspectModelBuilder.hpp"

#include "cards/CardKeyword.hpp"
#include "consumables/ConsumableRarity.hpp"
#include "dice/DiceExpression.hpp"
#include "drones/DroneActionDefinition.hpp"
#include "drones/DroneDefinition.hpp"
#include "drones/DroneId.hpp"
#include "effects/EffectDefinition.hpp"
#include "game/GameEvent.hpp"
#include "relics/RelicModifierDefinition.hpp"
#include "relics/RelicRarity.hpp"
#include "relics/RelicTriggerDefinition.hpp"
#include "statuses/StatusDefinition.hpp"
#include "statuses/StatusDurationRule.hpp"
#include "statuses/StatusType.hpp"

#include <algorithm>
#include <sstream>
#include <utility>

InspectModelBuilder::InspectModelBuilder(
    const ContentRegistry& content,
    const LocalizationManager& localization
)
    : content_(content),
      localization_(localization) {}

InspectPanelModel InspectModelBuilder::buildEnemy(const EnemyViewModel& enemy) const {
    InspectPanelModel model;
    model.header = enemy.name;

    model.entries.push_back(InspectEntry{
        rawText("inspect.enemy.health.name"),
        formatRawText("inspect.enemy.health.value",
            {{"current", std::to_string(enemy.currentHp)}, {"maximum", std::to_string(enemy.maxHp)}}
        )
    });

    if (!enemy.intentText.empty()) {
        model.entries.push_back(InspectEntry{
            rawText("inspect.enemy.intent"),
            enemy.intentText,
            InspectEntryStyle::Hint
        });
    }

    if (enemy.block > 0) {
        model.entries.push_back(InspectEntry{
            rawText("inspect.combat.block.name"),
            formatRawText("inspect.combat.block.value",
                {{"amount", std::to_string(enemy.block)}}
            )
        });
    }

    for (const StatusViewModel& status : enemy.statuses) {
        appendStatusEntry(model, status);
    }

    if (enemy.statuses.empty() && enemy.block <= 0 && enemy.intentText.empty()) {
        model.entries.push_back(InspectEntry{
            rawText("inspect.enemy.no_details.name"),
            rawText("inspect.enemy.no_details.description")
        });
    }

    return model;
}

InspectPanelModel InspectModelBuilder::buildPlayer(const PlayerViewModel& player) const {
    InspectPanelModel model;
    model.header = player.name;

    if (player.activeTurn) {
        model.entries.push_back(InspectEntry{
            rawText("inspect.player.active_turn.name"),
            rawText("inspect.player.active_turn.description"),
            InspectEntryStyle::Hint
        });
    }

    model.entries.push_back(InspectEntry{
        rawText("inspect.player.health.name"),
        formatRawText("inspect.player.health.value",
            {{"current", std::to_string(player.currentHp)}, {"maximum", std::to_string(player.maxHp)}}
        )
    });

    if (player.maxEnergy > 0) {
        model.entries.push_back(InspectEntry{
            rawText("inspect.player.energy.name"),
            formatRawText("inspect.player.energy.value",
                {{"current", std::to_string(player.currentEnergy)}, {"maximum", std::to_string(player.maxEnergy)}}
            )
        });
    }

    model.entries.push_back(InspectEntry{
        rawText("inspect.player.stress.name"),
        formatRawText("inspect.player.stress.value",
            {{"current", std::to_string(player.stress)}, {"maximum", std::to_string(player.maxStress)}}
        ),
        player.stress >= 100 ? InspectEntryStyle::Warning : InspectEntryStyle::Normal
    });

    if (player.block > 0) {
        model.entries.push_back(InspectEntry{
            rawText("inspect.combat.block.name"),
            formatRawText("inspect.combat.block.value",
                {{"amount", std::to_string(player.block)}}
            )
        });
    }

    for (const StatusViewModel& status : player.statuses) {
        appendStatusEntry(model, status);
    }

    for (const std::string& traitId : player.traitIds) {
        model.entries.push_back(InspectEntry{
            traitName(traitId),
            traitDescription(traitId),
            InspectEntryStyle::Hint
        });
    }

    if (player.statuses.empty() && player.traitIds.empty() && player.block <= 0 && !player.activeTurn) {
        model.entries.push_back(InspectEntry{
            rawText("inspect.player.no_effects.name"),
            rawText("inspect.player.no_effects.description")
        });
    }

    return model;
}

InspectPanelModel InspectModelBuilder::buildConsumable(const ConsumableViewModel& consumable) const {
    InspectPanelModel model;

    if (!consumable.filled) {
        model.header = rawText("inspect.consumable.empty.name");
        model.entries.push_back(InspectEntry{
            rawText("inspect.consumable.empty.name"),
            rawText("inspect.consumable.empty.description")
        });
        return model;
    }

    model.header = consumable.name;
    model.subheader = consumable.description;
    model.entries.push_back(InspectEntry{
        rawText("inspect.consumable.use_hint.name"),
        rawText("inspect.consumable.use_hint.description"),
        InspectEntryStyle::Hint
    });
    return model;
}

InspectPanelModel InspectModelBuilder::buildConsumable(const ConsumableDefinition& consumable) const {
    InspectPanelModel model;
    model.header = textOrFallback(consumable.nameTextId, consumable.id.value);
    model.subheader = textOrFallback(consumable.descriptionTextId, consumable.id.value);

    model.entries.push_back(InspectEntry{
        rawText("inspect.consumable.rarity.name"),
        consumableRarityText(consumable.rarity)
    });

    model.entries.push_back(InspectEntry{
        rawText("inspect.consumable.gold_cost.name"),
        formatRawText("inspect.consumable.gold_cost.value",
            {{"amount", std::to_string(consumable.goldCost)}}
        )
    });

    model.entries.push_back(InspectEntry{
        rawText("inspect.consumable.use_hint.name"),
        rawText("inspect.consumable.use_hint.description"),
        InspectEntryStyle::Hint
    });

    for (const EffectDefinition& effect : consumable.effects) {
        appendEffectEntry(model, "inspect.consumable.effects.name", effect);
    }

    return model;
}

InspectPanelModel InspectModelBuilder::buildRelic(const RelicDefinition& relic) const {
    InspectPanelModel model;
    model.header = textOrFallback(relic.nameTextId, relic.id.value);
    model.subheader = textOrFallback(relic.descriptionTextId, relic.id.value);

    model.entries.push_back(InspectEntry{
        rawText("inspect.relic.rarity.name"),
        relicRarityText(relic.rarity)
    });

    if (!relic.mechanicId.empty() && relic.mechanicId != "default") {
        model.entries.push_back(InspectEntry{
            rawText("inspect.relic.mechanic.name"),
            relic.mechanicId,
            InspectEntryStyle::Hint
        });
    }

    for (const RelicModifierDefinition& modifier : relic.modifiers) {
        model.entries.push_back(InspectEntry{
            rawText("inspect.relic.modifiers.name"),
            relicModifierSummary(modifier)
        });
    }

    for (const RelicTriggerDefinition& trigger : relic.triggers) {
        model.entries.push_back(InspectEntry{
            rawText("inspect.relic.triggers.name"),
            relicTriggerSummary(trigger)
        });
    }

    if (relic.modifiers.empty() && relic.triggers.empty()) {
        model.entries.push_back(InspectEntry{
            rawText("inspect.relic.modifiers.name"),
            rawText("inspect.relic.passive_only"),
            InspectEntryStyle::Warning
        });
    }

    return model;
}

InspectPanelModel InspectModelBuilder::buildDroneSlot(const DroneSlotViewModel& droneSlot) const {
    InspectPanelModel model;
    model.header = rawText("inspect.drone.slot.name");

    if (!droneSlot.filled) {
        model.subheader = droneSlot.name.empty()
            ? rawText("inspect.drone.empty.name")
            : droneSlot.name;
        model.entries.push_back(InspectEntry{
            rawText("inspect.drone.empty.name"),
            droneSlot.description.empty()
                ? rawText("inspect.drone.empty.description")
                : droneSlot.description
        });
        return model;
    }

    model.subheader = droneSlot.name;
    model.entries.push_back(InspectEntry{
        rawText("inspect.drone.active.name"),
        droneSlot.description
    });

    model.entries.push_back(InspectEntry{
        rawText("inspect.drone.activation_state.name"),
        droneSlot.cardActivationLabel.empty()
            ? rawText("inspect.drone.activation_state.unknown")
            : droneSlot.cardActivationLabel,
        droneSlot.cardActivationAvailable ? InspectEntryStyle::Hint : InspectEntryStyle::Normal
    });

    const DroneId droneId(droneSlot.type);
    if (!content_.drones().contains(droneId)) {
        model.entries.push_back(InspectEntry{
            rawText("inspect.drone.unknown.name"),
            rawText("inspect.drone.unknown.description"),
            InspectEntryStyle::Warning
        });
        return model;
    }

    const DroneDefinition& definition = content_.drones().get(droneId);
    if (definition.activeAction.has_value()) {
        model.entries.push_back(InspectEntry{
            rawText("inspect.drone.active_action.name"),
            droneActionSummary(*definition.activeAction),
            InspectEntryStyle::Hint
        });
    } else {
        model.entries.push_back(InspectEntry{
            rawText("inspect.drone.active_action.name"),
            rawText("inspect.drone.no_active_action"),
            InspectEntryStyle::Warning
        });
    }

    if (definition.passiveAction.has_value()) {
        model.entries.push_back(InspectEntry{
            rawText("inspect.drone.passive_action.name"),
            droneActionSummary(*definition.passiveAction),
            InspectEntryStyle::Hint
        });
    }

    return model;
}

InspectPanelModel InspectModelBuilder::buildCard(
    const CardDefinition& definition,
    const CardViewModel& card
) const {
    InspectPanelModel model;
    model.header = card.name;
    model.subheader = card.description;

    model.entries.push_back(InspectEntry{
        rawText("inspect.card.cost.name"),
        std::to_string(card.energyCost)
    });

    model.entries.push_back(InspectEntry{
        rawText("inspect.card.type.name"),
        rawTextOrFallback("card.type." + toString(definition.type), toString(definition.type))
    });

    model.entries.push_back(InspectEntry{
        rawText("inspect.card.rarity.name"),
        rawTextOrFallback("card.rarity." + toString(definition.rarity), toString(definition.rarity))
    });

    if (!card.ownerLabel.empty()) {
        model.entries.push_back(InspectEntry{
            rawText("inspect.card.owner.name"),
            card.ownerLabel
        });
    }

    model.entries.push_back(InspectEntry{
        rawText("inspect.card.upgrade.name"),
        card.upgraded
            ? rawText("inspect.card.upgrade.yes")
            : rawText("inspect.card.upgrade.no"),
        card.upgraded ? InspectEntryStyle::Hint : InspectEntryStyle::Normal
    });

    if (!card.playable && !card.unplayableReason.empty()) {
        model.entries.push_back(InspectEntry{
            rawText("inspect.card.playability.name"),
            card.unplayableReason,
            InspectEntryStyle::Warning
        });
    }

    for (const CardKeyword keyword : definition.keywords) {
        model.entries.push_back(InspectEntry{
            keywordName(keyword),
            keywordDescription(keyword),
            InspectEntryStyle::Hint
        });
    }

    for (const EffectDefinition& effect : definition.effects) {
        appendEffectEntry(model, "inspect.card.effect.name", effect);
    }

    if (definition.effects.empty() && definition.keywords.empty()) {
        model.entries.push_back(InspectEntry{
            rawText("inspect.card.no_details.name"),
            rawText("inspect.card.no_details.description")
        });
    }

    return model;
}

void InspectModelBuilder::appendStatusEntry(InspectPanelModel& model, const StatusViewModel& status) const {
    appendStatusEntry(model, status.id, status.amount);
}

void InspectModelBuilder::appendStatusEntry(InspectPanelModel& model, const std::string& statusId, const int amount) const {
    std::string title = statusName(statusId);
    if (amount > 0) {
        title = formatRawText("inspect.status.title_with_amount",
            {{"name", title}, {"amount", std::to_string(amount)}}
        );
    }

    std::string description = statusDescription(statusId);

    const std::string runtime = statusStackRuntimeDescription(statusId, amount);
    if (!runtime.empty()) {
        description += "\n" + runtime;
    }

    const std::string rule = statusRuleDescription(statusId);
    if (!rule.empty()) {
        description += "\n" + rule;
    }

    model.entries.push_back(InspectEntry{title, description});
}

void InspectModelBuilder::appendEffectEntry(
    InspectPanelModel& model,
    const std::string& titleTextId,
    const EffectDefinition& effect
) const {
    model.entries.push_back(InspectEntry{
        rawTextOrFallback(titleTextId, rawText("inspect.effect.unknown")),
        effectSummary(effect)
    });

    if (effect.statusId.has_value()) {
        appendStatusEntry(model, *effect.statusId, 0);
    }
}

std::string InspectModelBuilder::textOrId(const TextId& textId) const {
    if (localization_.hasText(textId)) {
        return localization_.get(textId);
    }

    return textId.value;
}

std::string InspectModelBuilder::textOrFallback(
    const TextId& textId,
    const std::string& fallback
) const {
    if (localization_.hasText(textId)) {
        return localization_.get(textId);
    }

    return fallback;
}

std::string InspectModelBuilder::rawText(const std::string& textId) const {
    return localization_.get(TextId(textId));
}

std::string InspectModelBuilder::rawTextOrFallback(
    const std::string& textId,
    const std::string& fallback
) const {
    return textOrFallback(TextId(textId), fallback);
}

std::string InspectModelBuilder::formatRawText(
    const std::string& textId,
    const std::vector<std::pair<std::string, std::string>>& variables
) const {
    std::string text = rawText(textId);
    for (const auto& [key, value] : variables) {
        const std::string marker = "{" + key + "}";
        std::size_t position = 0;
        while ((position = text.find(marker, position)) != std::string::npos) {
            text.replace(position, marker.size(), value);
            position += value.size();
        }
    }

    return text;
}

std::string InspectModelBuilder::keywordName(const CardKeyword keyword) const {
    const std::string key = "keyword." + toString(keyword) + ".name";
    return rawTextOrFallback(key, toString(keyword));
}

std::string InspectModelBuilder::keywordDescription(const CardKeyword keyword) const {
    const std::string key = "keyword." + toString(keyword) + ".description";
    if (localization_.hasText(TextId(key))) {
        return localization_.get(TextId(key));
    }

    return rawText("keyword.unknown.description");
}

std::string InspectModelBuilder::traitName(const std::string& traitId) const {
    return rawTextOrFallback("trait." + traitId + ".name", traitId);
}

std::string InspectModelBuilder::traitDescription(const std::string& traitId) const {
    return rawTextOrFallback("trait." + traitId + ".description", traitId);
}

std::string InspectModelBuilder::statusName(const std::string& statusId) const {
    const StatusId id(statusId);

    if (content_.statuses().contains(id)) {
        const StatusDefinition& definition = content_.statuses().get(id);
        return textOrFallback(definition.nameTextId, statusId);
    }

    return statusId;
}

std::string InspectModelBuilder::statusDescription(const std::string& statusId) const {
    const StatusId id(statusId);

    if (content_.statuses().contains(id)) {
        const StatusDefinition& definition = content_.statuses().get(id);
        if (localization_.hasText(definition.descriptionTextId)) {
            return localization_.get(definition.descriptionTextId);
        }

        return rawText("inspect.status.unknown");
    }

    return rawText("inspect.status.unknown");
}

std::string InspectModelBuilder::statusStackRuntimeDescription(const std::string& statusId, const int amount) const {
    if (statusId != "poison" || amount <= 0) {
        return {};
    }

    return formatRawText("inspect.status.poison_runtime",
        {
            {"amount", std::to_string(amount)},
            {"damage", std::to_string(amount)},
            {"remaining", std::to_string(std::max(0, amount - 1))}
        }
    );
}

std::string InspectModelBuilder::statusRuleDescription(const std::string& statusId) const {
    const StatusId id(statusId);
    if (!content_.statuses().contains(id)) {
        return {};
    }

    const StatusDefinition& definition = content_.statuses().get(id);
    std::vector<std::string> parts;
    parts.push_back(formatRawText("inspect.status.type.value",
        {{"type", statusTypeText(definition.type)}}
    ));
    parts.push_back(formatRawText("inspect.status.duration.value",
        {{"duration", statusDurationText(definition.durationRule)}}
    ));

    if (!definition.endTurnEffect.empty()) {
        parts.push_back(formatRawText("inspect.status.end_turn_effect.value",
            {{"effect", statusEndTurnEffectText(definition.endTurnEffect)}}
        ));
    }

    if (definition.decreaseAfterTrigger) {
        parts.push_back(rawText("inspect.status.decrease_after_trigger"));
    }

    std::ostringstream out;
    for (std::size_t i = 0; i < parts.size(); ++i) {
        if (i > 0) {
            out << "\n";
        }
        out << parts[i];
    }
    return out.str();
}

std::string InspectModelBuilder::statusTypeText(const StatusType type) const {
    return rawTextOrFallback("status.type." + toString(type), toString(type));
}

std::string InspectModelBuilder::statusDurationText(const StatusDurationRule rule) const {
    return rawTextOrFallback("status.duration." + toString(rule), toString(rule));
}

std::string InspectModelBuilder::statusEndTurnEffectText(const std::string& effectId) const {
    if (effectId.empty()) {
        return {};
    }

    return rawTextOrFallback("status.end_turn_effect." + effectId, effectId);
}

std::string InspectModelBuilder::effectSummary(const EffectDefinition& effect) const {
    std::ostringstream out;

    switch (effect.type) {
        case EffectType::Damage:
            out << rawText("inspect.effect.damage");
            break;
        case EffectType::Block:
            out << rawText("inspect.effect.block");
            break;
        case EffectType::Heal:
            out << rawText("inspect.effect.heal");
            break;
        case EffectType::DrawCards:
            out << rawText("inspect.effect.draw_cards");
            break;
        case EffectType::DiscardCards:
            out << rawText("inspect.effect.discard_cards");
            break;
        case EffectType::ApplyStatus:
            out << rawText("inspect.effect.apply_status");
            break;
        case EffectType::GainEnergy:
            out << rawText("inspect.effect.gain_energy");
            break;
        case EffectType::GainStress:
            out << rawText("inspect.effect.gain_stress");
            break;
        case EffectType::LoseEnergy:
            out << rawText("inspect.effect.lose_energy");
            break;
        case EffectType::LoseStress:
            out << rawText("inspect.effect.lose_stress");
            break;
        case EffectType::LoseHp:
            out << rawText("inspect.effect.lose_hp");
            break;
        case EffectType::EnterStance:
            out << rawText("inspect.effect.enter_stance");
            break;
        case EffectType::SummonDrone:
            out << rawText("inspect.effect.summon_drone");
            if (effect.statusId.has_value()) {
                const DroneId droneId(*effect.statusId);
                if (content_.drones().contains(droneId)) {
                    const DroneDefinition& definition = content_.drones().get(droneId);
                    out << ": " << textOrFallback(definition.nameTextId, *effect.statusId);
                } else {
                    out << ": " << *effect.statusId;
                }
            }
            return out.str();
        case EffectType::UseDrone:
            return rawText("inspect.effect.use_drone");
    }

    out << ": " << valueText(effect.value);

    if (effect.repeatCount > 1) {
        out << " x" << effect.repeatCount;
    }

    if (effect.statusId.has_value()) {
        out << " " << statusName(*effect.statusId);
    }

    out << " -> " << targetText(effect.target);
    return out.str();
}

std::string InspectModelBuilder::droneActionSummary(const DroneActionDefinition& action) const {
    std::ostringstream out;
    if (!action.logTextId.empty() || !action.fallbackLog.empty()) {
        out << rawTextOrFallback(action.logTextId, action.fallbackLog.empty() ? action.logTextId : action.fallbackLog);
    }

    for (std::size_t i = 0; i < action.effects.size(); ++i) {
        if (out.tellp() > 0 || i > 0) {
            out << "\n";
        }
        out << effectSummary(action.effects[i]);
    }

    return out.str();
}

std::string InspectModelBuilder::relicRarityText(const RelicRarity rarity) const {
    return rawTextOrFallback("relic.rarity." + toString(rarity), toString(rarity));
}

std::string InspectModelBuilder::consumableRarityText(const ConsumableRarity rarity) const {
    return rawTextOrFallback("consumable.rarity." + toString(rarity), toString(rarity));
}

std::string InspectModelBuilder::relicModifierSummary(const RelicModifierDefinition& modifier) const {
    std::ostringstream out;
    out << rawTextOrFallback("relic.modifier." + toString(modifier.type), toString(modifier.type));

    if (modifier.multiplier != 1.0) {
        out << " x" << modifier.multiplier;
    }

    if (modifier.amount != 0) {
        out << " " << (modifier.amount > 0 ? "+" : "") << modifier.amount;
    }

    out << " (" << (modifier.playerOnly
        ? rawText("inspect.relic.player_only")
        : rawText("inspect.relic.affects_all")) << ")";
    return out.str();
}

std::string InspectModelBuilder::relicTriggerSummary(const RelicTriggerDefinition& trigger) const {
    std::ostringstream out;
    out << rawTextOrFallback("game_event." + toString(trigger.eventType), toString(trigger.eventType));

    if (trigger.everyNTurns > 0) {
        out << ", " << formatRawText("inspect.relic.every_n_turns",
            {{"count", std::to_string(trigger.everyNTurns)}}
        );
    }

    if (trigger.oncePerCombat) {
        out << ", " << rawText("inspect.relic.once_per_combat");
    }

    if (!trigger.effects.empty()) {
        out << ": ";
        for (std::size_t i = 0; i < trigger.effects.size(); ++i) {
            if (i > 0) {
                out << "; ";
            }
            out << effectSummary(trigger.effects[i]);
        }
    }

    return out.str();
}

std::string InspectModelBuilder::targetText(const EffectTarget target) const {
    switch (target) {
        case EffectTarget::Self:
            return rawText("inspect.target.self");
        case EffectTarget::SingleEnemy:
            return rawText("inspect.target.single_enemy");
        case EffectTarget::AllEnemies:
            return rawText("inspect.target.all_enemies");
        case EffectTarget::RandomEnemy:
            return rawText("inspect.target.random_enemy");
        case EffectTarget::Ally:
            return rawText("inspect.target.ally");
        case EffectTarget::AllAllies:
            return rawText("inspect.target.all_allies");
        case EffectTarget::RandomAlly:
            return rawText("inspect.target.random_ally");
    }

    return rawText("inspect.target.unknown");
}

std::string InspectModelBuilder::valueText(const EffectValue& value) const {
    if (value.isDice()) {
        return ::toString(value.diceExpression());
    }

    return std::to_string(value.fixedAmount());
}
