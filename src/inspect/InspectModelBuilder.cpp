#include "InspectModelBuilder.hpp"

#include "cards/CardKeyword.hpp"
#include "effects/EffectDefinition.hpp"
#include "statuses/StatusDefinition.hpp"
#include "dice/DiceExpression.hpp"
#include "drones/DroneDefinition.hpp"
#include "drones/DroneId.hpp"
#include "relics/RelicModifierDefinition.hpp"
#include "relics/RelicRarity.hpp"
#include "relics/RelicTriggerDefinition.hpp"
#include "consumables/ConsumableRarity.hpp"
#include "game/GameEvent.hpp"

#include <sstream>

InspectModelBuilder::InspectModelBuilder(
    const ContentRegistry& content,
    const LocalizationManager& localization
)
    : content_(content),
      localization_(localization) {}

InspectPanelModel InspectModelBuilder::buildEnemy(const EnemyViewModel& enemy) const {
    InspectPanelModel model;
    model.header = enemy.name;

    if (!enemy.intentText.empty()) {
        model.entries.push_back(InspectEntry{
            rawTextOrFallback("inspect.enemy.intent", "Intent"),
            enemy.intentText
        });
    }

    if (enemy.block > 0) {
        model.entries.push_back(InspectEntry{
            rawTextOrFallback("inspect.combat.block.name", "Block"),
            rawTextOrFallback(
                "inspect.combat.block.description",
                "Prevents incoming damage before HP is lost."
            )
        });
    }

    for (const StatusViewModel& status : enemy.statuses) {
        std::string title = status.name;
        if (status.amount > 0) {
            title += ": " + std::to_string(status.amount);
        }

        model.entries.push_back(InspectEntry{
            title,
            statusDescription(status.id)
        });
    }

    if (model.entries.empty()) {
        model.entries.push_back(InspectEntry{
            rawTextOrFallback("inspect.enemy.no_details.name", "No visible effects"),
            rawTextOrFallback(
                "inspect.enemy.no_details.description",
                "This enemy currently has no visible intent details or statuses."
            )
        });
    }

    return model;
}

InspectPanelModel InspectModelBuilder::buildPlayer(const PlayerViewModel& player) const {
    InspectPanelModel model;
    model.header = player.name;

    model.entries.push_back(InspectEntry{
        rawTextOrFallback("inspect.player.hp.name", "HP"),
        std::to_string(player.currentHp) + "/" + std::to_string(player.maxHp)
    });

    if (player.maxEnergy > 0) {
        model.entries.push_back(InspectEntry{
            rawTextOrFallback("inspect.player.energy.name", "Energy"),
            std::to_string(player.currentEnergy) + "/" + std::to_string(player.maxEnergy) + "\n" +
                rawTextOrFallback(
                    "inspect.player.energy.description",
                    "Energy is spent to play cards belonging to this actor."
                )
        });
    }

    if (player.block > 0) {
        model.entries.push_back(InspectEntry{
            rawTextOrFallback("inspect.combat.block.name", "Block"),
            rawTextOrFallback(
                "inspect.combat.block.description",
                "Prevents incoming damage before HP is lost."
            )
        });
    }

    for (const StatusViewModel& status : player.statuses) {
        std::string title = status.name;
        if (status.amount > 0) {
            title += ": " + std::to_string(status.amount);
        }

        model.entries.push_back(InspectEntry{
            title,
            statusDescription(status.id)
        });
    }

    if (model.entries.size() <= 3) {
        model.entries.push_back(InspectEntry{
            rawTextOrFallback("inspect.player.no_effects.name", "No active effects"),
            rawTextOrFallback(
                "inspect.player.no_effects.description",
                "This actor currently has no active statuses or special visible effects."
            )
        });
    }

    return model;
}

InspectPanelModel InspectModelBuilder::buildConsumable(const ConsumableViewModel& consumable) const {
    InspectPanelModel model;

    if (!consumable.filled) {
        model.header = rawTextOrFallback("inspect.consumable.empty.name", "Empty slot");
        model.entries.push_back(InspectEntry{
            rawTextOrFallback("inspect.consumable.empty.name", "Empty slot"),
            rawTextOrFallback(
                "inspect.consumable.empty.description",
                "This consumable slot is empty."
            )
        });
        return model;
    }

    model.header = consumable.name;
    model.subheader = consumable.description;
    model.entries.push_back(InspectEntry{
        rawTextOrFallback("inspect.consumable.use_hint.name", "Use"),
        rawTextOrFallback(
            "inspect.consumable.use_hint.description",
            "Left-click uses this consumable."
        )
    });
    return model;
}


InspectPanelModel InspectModelBuilder::buildConsumable(const ConsumableDefinition& consumable) const {
    InspectPanelModel model;
    model.header = textOrFallback(consumable.nameTextId, consumable.id.value);
    model.subheader = textOrFallback(consumable.descriptionTextId, consumable.id.value);

    model.entries.push_back(InspectEntry{
        rawTextOrFallback("inspect.consumable.rarity.name", "Rarity"),
        consumableRarityText(consumable.rarity)
    });

    std::string goldCost = rawTextOrFallback(
        "inspect.consumable.gold_cost.value",
        "{amount} gold"
    );
    const std::string amountMarker = "{amount}";
    const std::size_t amountPosition = goldCost.find(amountMarker);
    if (amountPosition != std::string::npos) {
        goldCost.replace(amountPosition, amountMarker.size(), std::to_string(consumable.goldCost));
    }

    model.entries.push_back(InspectEntry{
        rawTextOrFallback("inspect.consumable.gold_cost.name", "Gold cost"),
        goldCost
    });

    model.entries.push_back(InspectEntry{
        rawTextOrFallback("inspect.consumable.use_hint.name", "Use"),
        rawTextOrFallback(
            "inspect.consumable.use_hint.description",
            "Left-click opens a use confirmation window."
        )
    });

    if (!consumable.effects.empty()) {
        for (const EffectDefinition& effect : consumable.effects) {
            model.entries.push_back(InspectEntry{
                rawTextOrFallback("inspect.consumable.effects.name", "Effects"),
                effectSummary(effect)
            });

            if (effect.statusId.has_value()) {
                model.entries.push_back(InspectEntry{
                    statusName(*effect.statusId),
                    statusDescription(*effect.statusId)
                });
            }
        }
    }

    return model;
}

InspectPanelModel InspectModelBuilder::buildRelic(const RelicDefinition& relic) const {
    InspectPanelModel model;
    model.header = textOrFallback(relic.nameTextId, relic.id.value);
    model.subheader = textOrFallback(relic.descriptionTextId, relic.id.value);

    model.entries.push_back(InspectEntry{
        rawTextOrFallback("inspect.relic.rarity.name", "Rarity"),
        relicRarityText(relic.rarity)
    });

    if (!relic.mechanicId.empty() && relic.mechanicId != "default") {
        model.entries.push_back(InspectEntry{
            rawTextOrFallback("inspect.relic.mechanic.name", "Mechanic"),
            relic.mechanicId
        });
    }

    if (!relic.modifiers.empty()) {
        for (const RelicModifierDefinition& modifier : relic.modifiers) {
            model.entries.push_back(InspectEntry{
                rawTextOrFallback("inspect.relic.modifiers.name", "Modifiers"),
                relicModifierSummary(modifier)
            });
        }
    }

    if (!relic.triggers.empty()) {
        for (const RelicTriggerDefinition& trigger : relic.triggers) {
            model.entries.push_back(InspectEntry{
                rawTextOrFallback("inspect.relic.triggers.name", "Triggers"),
                relicTriggerSummary(trigger)
            });
        }
    }

    if (relic.modifiers.empty() && relic.triggers.empty()) {
        model.entries.push_back(InspectEntry{
            rawTextOrFallback("inspect.relic.modifiers.name", "Modifiers"),
            rawTextOrFallback(
                "inspect.relic.passive_only",
                "This relic has no parsed modifiers or triggers yet. The description above is the source of truth."
            )
        });
    }

    return model;
}

InspectPanelModel InspectModelBuilder::buildDroneSlot(const DroneSlotViewModel& droneSlot) const {
    InspectPanelModel model;
    model.header = rawTextOrFallback("inspect.drone.slot.name", "Drone slot");

    if (!droneSlot.filled) {
        model.subheader = droneSlot.name.empty()
            ? rawTextOrFallback("inspect.drone.empty.name", "Empty slot")
            : droneSlot.name;
        model.entries.push_back(InspectEntry{
            rawTextOrFallback("inspect.drone.empty.name", "Empty slot"),
            droneSlot.description.empty()
                ? rawTextOrFallback("inspect.drone.empty.description", "This drone slot is empty.")
                : droneSlot.description
        });
        return model;
    }

    model.subheader = droneSlot.name;
    model.entries.push_back(InspectEntry{
        rawTextOrFallback("inspect.drone.active.name", "Active drone"),
        droneSlot.description
    });
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
        rawTextOrFallback("inspect.card.cost.name", "Cost"),
        std::to_string(card.energyCost)
    });

    model.entries.push_back(InspectEntry{
        rawTextOrFallback("inspect.card.type.name", "Type"),
        rawTextOrFallback("card.type." + toString(definition.type), toString(definition.type))
    });

    model.entries.push_back(InspectEntry{
        rawTextOrFallback("inspect.card.rarity.name", "Rarity"),
        rawTextOrFallback("card.rarity." + toString(definition.rarity), toString(definition.rarity))
    });

    if (!card.ownerLabel.empty()) {
        model.entries.push_back(InspectEntry{
            rawTextOrFallback("inspect.card.owner.name", "Owner"),
            card.ownerLabel
        });
    }

    model.entries.push_back(InspectEntry{
        rawTextOrFallback("inspect.card.upgrade.name", "Upgrade"),
        card.upgraded
            ? rawTextOrFallback("inspect.card.upgrade.yes", "This card is upgraded.")
            : rawTextOrFallback("inspect.card.upgrade.no", "This card is not upgraded.")
    });

    if (!card.playable && !card.unplayableReason.empty()) {
        model.entries.push_back(InspectEntry{
            rawTextOrFallback("inspect.card.playability.name", "Playability"),
            card.unplayableReason
        });
    }

    if (!definition.keywords.empty()) {
        for (const CardKeyword keyword : definition.keywords) {
            model.entries.push_back(InspectEntry{
                keywordName(keyword),
                keywordDescription(keyword)
            });
        }
    }

    for (const EffectDefinition& effect : definition.effects) {
        model.entries.push_back(InspectEntry{
            rawTextOrFallback("inspect.card.effect.name", "Effect"),
            effectSummary(effect)
        });

        if (effect.statusId.has_value()) {
            model.entries.push_back(InspectEntry{
                statusName(*effect.statusId),
                statusDescription(*effect.statusId)
            });
        }
    }

    if (definition.effects.empty() && definition.keywords.empty()) {
        model.entries.push_back(InspectEntry{
            rawTextOrFallback("inspect.card.no_details.name", "No details"),
            rawTextOrFallback(
                "inspect.card.no_details.description",
                "This card has no keywords or linked mechanics."
            )
        });
    }

    return model;
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

std::string InspectModelBuilder::rawTextOrFallback(
    const std::string& textId,
    const std::string& fallback
) const {
    return textOrFallback(TextId(textId), fallback);
}

std::string InspectModelBuilder::keywordName(const CardKeyword keyword) const {
    const std::string key = "keyword." + toString(keyword) + ".name";
    return rawTextOrFallback(key, toString(keyword));
}

std::string InspectModelBuilder::keywordDescription(const CardKeyword keyword) const {
    const std::string key = "keyword." + toString(keyword) + ".description";
    return rawTextOrFallback(key, "No keyword description.");
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
        return textOrFallback(definition.descriptionTextId, "No status description.");
    }

    return rawTextOrFallback("inspect.status.unknown", "No status description.");
}

std::string InspectModelBuilder::effectSummary(const EffectDefinition& effect) const {
    std::ostringstream out;

    switch (effect.type) {
        case EffectType::Damage:
            out << rawTextOrFallback("inspect.effect.damage", "Deals damage");
            break;
        case EffectType::Block:
            out << rawTextOrFallback("inspect.effect.block", "Gains block");
            break;
        case EffectType::Heal:
            out << rawTextOrFallback("inspect.effect.heal", "Heals");
            break;
        case EffectType::DrawCards:
            out << rawTextOrFallback("inspect.effect.draw_cards", "Draws cards");
            break;
        case EffectType::DiscardCards:
            out << rawTextOrFallback("inspect.effect.discard_cards", "Discards cards");
            break;
        case EffectType::ApplyStatus:
            out << rawTextOrFallback("inspect.effect.apply_status", "Applies status");
            break;
        case EffectType::GainEnergy:
            out << rawTextOrFallback("inspect.effect.gain_energy", "Gains energy");
            break;
        case EffectType::GainStress:
            out << rawTextOrFallback("inspect.effect.gain_stress", "Gains stress");
            break;
        case EffectType::LoseEnergy:
            out << rawTextOrFallback("inspect.effect.lose_energy", "Loses energy");
            break;
        case EffectType::LoseStress:
            out << rawTextOrFallback("inspect.effect.lose_stress", "Loses stress");
            break;
        case EffectType::LoseHp:
            out << rawTextOrFallback("inspect.effect.lose_hp", "Loses HP");
            break;
        case EffectType::EnterStance:
            out << rawTextOrFallback("inspect.effect.enter_stance", "Enters stance");
            break;
        case EffectType::SummonDrone:
            out << rawTextOrFallback("inspect.effect.summon_drone", "Summons drone");
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
            return rawTextOrFallback("inspect.effect.use_drone", "Uses the oldest drone");
        default:
            out << rawTextOrFallback("inspect.effect.unknown", "Effect");
            break;
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
        ? rawTextOrFallback("inspect.relic.player_only", "player only")
        : rawTextOrFallback("inspect.relic.affects_all", "affects all")) << ")";
    return out.str();
}

std::string InspectModelBuilder::relicTriggerSummary(const RelicTriggerDefinition& trigger) const {
    std::ostringstream out;
    out << rawTextOrFallback("game_event." + toString(trigger.eventType), toString(trigger.eventType));

    if (trigger.everyNTurns > 0) {
        std::string cadence = rawTextOrFallback("inspect.relic.every_n_turns", "every {count} turns");
        const std::string marker = "{count}";
        const std::size_t markerPosition = cadence.find(marker);
        if (markerPosition != std::string::npos) {
            cadence.replace(markerPosition, marker.size(), std::to_string(trigger.everyNTurns));
        }
        out << ", " << cadence;
    }

    if (trigger.oncePerCombat) {
        out << ", " << rawTextOrFallback("inspect.relic.once_per_combat", "once per combat");
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
            return rawTextOrFallback("inspect.target.self", "self");
        case EffectTarget::SingleEnemy:
            return rawTextOrFallback("inspect.target.single_enemy", "single enemy");
        case EffectTarget::AllEnemies:
            return rawTextOrFallback("inspect.target.all_enemies", "all enemies");
        case EffectTarget::RandomEnemy:
            return rawTextOrFallback("inspect.target.random_enemy", "random enemy");
        case EffectTarget::Ally:
            return rawTextOrFallback("inspect.target.ally", "ally");
        case EffectTarget::AllAllies:
            return rawTextOrFallback("inspect.target.all_allies", "all allies");
        case EffectTarget::RandomAlly:
            return rawTextOrFallback("inspect.target.random_ally", "random ally");
    }

    return "target";
}

std::string InspectModelBuilder::valueText(const EffectValue& value) const {
    if (value.isDice()) {
        return ::toString(value.diceExpression());
    }

    return std::to_string(value.fixedAmount());
}
