#include "CombatViewModelBuilder.hpp"
#include "combat/BossPhaseRules.hpp"

#include "drones/DroneDefinition.hpp"
#include "drones/DroneId.hpp"
#include "ui/EnemyIntentPresentation.hpp"
#include "run/StressPsychopathRules.hpp"
#include "statuses/StatusType.hpp"

#include <algorithm>
#include <cstdint>
#include <initializer_list>
#include <iterator>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <unordered_map>

namespace {
constexpr const char* stanceFlameStatusId = "stance_flame";
constexpr const char* stanceAshStatusId = "stance_ash";
constexpr const char* stanceSmokeStatusId = "stance_smoke";

bool isStanceStatus(const std::string& statusId) {
    return statusId == stanceFlameStatusId ||
           statusId == stanceAshStatusId ||
           statusId == stanceSmokeStatusId;
}

std::string words(std::initializer_list<std::string_view> tokens) {
    std::string result;
    for (const std::string_view token : tokens) {
        if (!result.empty()) {
            result.push_back(' ');
        }
        result.append(token.data(), token.size());
    }
    return result;
}

bool equalsWords(const std::string& text, std::initializer_list<std::string_view> tokens) {
    return text == words(tokens);
}

std::string localized(
    const LocalizationManager& localization,
    const std::string& textId
) {
    return localization.get(TextId(textId));
}

std::string localizedOrRaw(
    const LocalizationManager& localization,
    const std::string& textId,
    const std::string& raw
) {
    if (localization.hasText(TextId(textId))) {
        return localization.get(TextId(textId));
    }

    return raw;
}

std::string localizedFormat(
    const LocalizationManager& localization,
    const std::string& textId,
    const TextFormatter::Variables& variables
) {
    return localization.format(TextId(textId), variables);
}

std::string phaseLabel(
    const LocalizationManager& localization,
    const CombatPhase phase
) {
    switch (phase) {
        case CombatPhase::NotStarted:
            return localized(localization, "combat.phase.not_started");
        case CombatPhase::PlayerTurn:
            return localized(localization, "combat.phase.player_turn");
        case CombatPhase::EnemyTurn:
            return localized(localization, "combat.phase.enemy_turn");
        case CombatPhase::Won:
            return localized(localization, "combat.phase.won");
        case CombatPhase::Lost:
            return localized(localization, "combat.phase.lost");
    }

    return localized(localization, "combat.phase.unknown");
}

std::string intentBaseLabel(
    const LocalizationManager& localization,
    const EnemyIntentType type
) {
    switch (type) {
        case EnemyIntentType::Attack:
            return localized(localization, "intent.attack.name");
        case EnemyIntentType::Block:
            return localized(localization, "intent.block.name");
        case EnemyIntentType::Buff:
            return localized(localization, "intent.buff.name");
        case EnemyIntentType::Debuff:
            return localized(localization, "intent.debuff.name");
        case EnemyIntentType::Special:
            return localized(localization, "intent.special.name");
        case EnemyIntentType::Unknown:
            return localized(localization, "intent.unknown.name");
    }

    return localized(localization, "intent.unknown.name");
}

std::string numericRangeText(const int minimum, const int maximum) {
    if (minimum == maximum) {
        return std::to_string(maximum);
    }

    return std::to_string(minimum) + "-" + std::to_string(maximum);
}

std::string valueRangeText(
    const LocalizationManager& localization,
    const EnemyIntent& intent
) {
    if (intent.valueMax <= 0 && intent.valueMin <= 0) {
        return {};
    }

    const std::string value = numericRangeText(intent.valueMin, intent.valueMax);
    if (intent.hitCount > 1) {
        return localizedFormat(
            localization,
            "intent.value.multi_hit",
            {{"value", value}, {"hits", std::to_string(intent.hitCount)}}
        );
    }

    return value;
}

std::string intentLabel(
    const LocalizationManager& localization,
    const EnemyIntent& intent
) {
    const std::string base = intentBaseLabel(localization, intent.type);
    const std::string value = valueRangeText(localization, intent);

    if (value.empty() || intent.type == EnemyIntentType::Buff ||
        intent.type == EnemyIntentType::Debuff || intent.type == EnemyIntentType::Special ||
        intent.type == EnemyIntentType::Unknown) {
        return base;
    }

    return base + " " + value;
}

int intentDangerLevel(const EnemyIntent& intent, const int referenceHp) {
    if (intent.type != EnemyIntentType::Attack || referenceHp <= 0) {
        return 0;
    }

    const int hitCount = std::max(1, intent.hitCount);
    const int maximumDamage = std::max(0, intent.valueMax) * hitCount;
    if (maximumDamage >= referenceHp) {
        return 3;
    }
    if (maximumDamage * 2 >= referenceHp) {
        return 2;
    }
    if (maximumDamage * 4 >= referenceHp) {
        return 1;
    }

    return 0;
}

std::string statusName(
    const LocalizationManager& localization,
    const StatusDatabase& statuses,
    const std::string& statusIdText
);

std::string targetScopeText(
    const LocalizationManager& localization,
    const EffectTarget target
) {
    switch (target) {
        case EffectTarget::Self:
            return localized(localization, "intent.target.self");
        case EffectTarget::Ally:
            return localized(localization, "intent.target.single_player");
        case EffectTarget::AllAllies:
            return localized(localization, "intent.target.all_players");
        case EffectTarget::RandomAlly:
            return localized(localization, "intent.target.random_player");
        case EffectTarget::SingleEnemy:
            return localized(localization, "intent.target.single_enemy");
        case EffectTarget::AllEnemies:
            return localized(localization, "intent.target.all_enemies");
        case EffectTarget::RandomEnemy:
            return localized(localization, "intent.target.random_enemy");
    }

    return localized(localization, "intent.target.unknown");
}

std::string repeatedValueText(const EnemyIntentEffectSummary& summary) {
    const std::string value = numericRangeText(summary.valueMin, summary.valueMax);
    if (summary.repeatCount <= 1) {
        return value;
    }

    const int totalMin = summary.valueMin * summary.repeatCount;
    const int totalMax = summary.valueMax * summary.repeatCount;
    return value + " × " + std::to_string(summary.repeatCount) +
           " = " + numericRangeText(totalMin, totalMax);
}

std::string effectSummaryLine(
    const LocalizationManager& localization,
    const StatusDatabase& statuses,
    const EnemyIntentEffectSummary& summary
) {
    const std::string target = targetScopeText(localization, summary.target);
    const std::string value = repeatedValueText(summary);

    switch (summary.type) {
        case EffectType::Damage:
            return localizedFormat(
                localization,
                "intent.summary.damage",
                {{"damage", value}, {"target", target}}
            );

        case EffectType::Block:
            return localizedFormat(
                localization,
                "intent.summary.block",
                {{"block", value}, {"target", target}}
            );

        case EffectType::Heal:
            return localizedFormat(
                localization,
                "intent.summary.heal",
                {{"heal", value}, {"target", target}}
            );

        case EffectType::ApplyStatus:
        case EffectType::EnterStance:
            return localizedFormat(
                localization,
                "intent.summary.status",
                {
                    {"status", statusName(localization, statuses, summary.statusId)},
                    {"amount", value},
                    {"target", target}
                }
            );

        case EffectType::LoseEnergy:
            return localizedFormat(
                localization,
                "intent.summary.lose_energy",
                {{"energy", value}, {"target", target}}
            );

        case EffectType::GainStress:
            return localizedFormat(
                localization,
                "intent.summary.gain_stress",
                {{"stress", value}, {"target", target}}
            );

        case EffectType::PrimeStressBreakdown:
            return localizedFormat(
                localization,
                "intent.summary.prime_stress_breakdown",
                {{"type", localized(localization, "breakdown.type." + summary.statusId)}, {"target", target}}
            );

        case EffectType::LoseStress:
            return localizedFormat(
                localization,
                "intent.summary.lose_stress",
                {{"stress", value}, {"target", target}}
            );

        case EffectType::LoseHp:
            return localizedFormat(
                localization,
                "intent.summary.lose_hp",
                {{"hp", value}, {"target", target}}
            );

        case EffectType::DrawCards:
            return localizedFormat(
                localization,
                "intent.summary.draw_cards",
                {{"cards", value}, {"target", target}}
            );

        case EffectType::DiscardCards:
            return localizedFormat(
                localization,
                "intent.summary.discard_cards",
                {{"cards", value}, {"target", target}}
            );

        case EffectType::RecoverCards:
            return localizedFormat(
                localization,
                "intent.summary.recover_cards",
                {{"cards", value}, {"target", target}}
            );

        case EffectType::GainEnergy:
            return localizedFormat(
                localization,
                "intent.summary.gain_energy",
                {{"energy", value}, {"target", target}}
            );

        case EffectType::SpendStressDamage:
        case EffectType::SpendStressBlock:
        case EffectType::SpendStressEnergy:
        case EffectType::SpendStressDraw:
        case EffectType::SummonDrone:
        case EffectType::UseDrone:
            return localizedFormat(
                localization,
                "intent.summary.special_effect",
                {{"target", target}}
            );
    }

    return localizedFormat(
        localization,
        "intent.summary.special_effect",
        {{"target", target}}
    );
}

std::string baseIntentDescription(
    const LocalizationManager& localization,
    const EnemyIntent& intent
) {
    switch (intent.type) {
        case EnemyIntentType::Attack:
            if (intent.hitCount > 1) {
                const int totalMin = intent.valueMin * intent.hitCount;
                const int totalMax = intent.valueMax * intent.hitCount;
                return localizedFormat(
                    localization,
                    "intent.attack.detail_multi",
                    {
                        {"damage", numericRangeText(intent.valueMin, intent.valueMax)},
                        {"hits", std::to_string(intent.hitCount)},
                        {"total", numericRangeText(totalMin, totalMax)}
                    }
                );
            }

            if (intent.valueMax > 0 || intent.valueMin > 0) {
                return localizedFormat(
                    localization,
                    "intent.attack.detail",
                    {{"damage", numericRangeText(intent.valueMin, intent.valueMax)}}
                );
            }

            return localized(localization, "intent.attack.description");

        case EnemyIntentType::Block:
            if (intent.valueMax > 0 || intent.valueMin > 0) {
                return localizedFormat(
                    localization,
                    "intent.block.detail",
                    {{"block", numericRangeText(intent.valueMin, intent.valueMax)}}
                );
            }

            return localized(localization, "intent.block.description");

        case EnemyIntentType::Buff:
            return localized(localization, "intent.buff.description");
        case EnemyIntentType::Debuff:
            return localized(localization, "intent.debuff.description");
        case EnemyIntentType::Special:
            return localized(localization, "intent.special.description");
        case EnemyIntentType::Unknown:
            return localized(localization, "intent.unknown.description");
    }

    return localized(localization, "intent.unknown.description");
}

std::string intentDetailText(
    const LocalizationManager& localization,
    const StatusDatabase& statuses,
    const EnemyIntent& intent
) {
    std::ostringstream stream;
    stream << baseIntentDescription(localization, intent);

    if (!intent.effectSummaries.empty()) {
        stream << "\n" << localized(localization, "intent.summary.effects_header");
        for (const EnemyIntentEffectSummary& summary : intent.effectSummaries) {
            stream << "\n• " << effectSummaryLine(localization, statuses, summary);
        }
    }

    return stream.str();
}

std::string variableOrFallback(
    const CombatLogEntry& entry,
    const std::string& name,
    const std::string& fallback = {}
) {
    const auto iterator = entry.variables.find(name);
    if (iterator == entry.variables.end()) {
        return fallback;
    }

    return iterator->second;
}


std::string localizedTextIdVariable(
    const LocalizationManager& localization,
    const CombatLogEntry& entry,
    const std::string& textIdVariable,
    const std::string& fallbackVariable
) {
    const std::string textId = variableOrFallback(entry, textIdVariable);
    if (!textId.empty() && localization.hasText(TextId(textId))) {
        return localization.get(TextId(textId));
    }

    return variableOrFallback(entry, fallbackVariable);
}

std::string localizedEntityVariable(
    const LocalizationManager& localization,
    const CombatLogEntry& entry,
    const std::string& prefix
) {
    const std::string localizedName = localizedTextIdVariable(
        localization,
        entry,
        prefix + "_text_id",
        prefix
    );
    return localizedName.empty()
        ? localized(localization, "combat.log.unknown_entity")
        : localizedName;
}

CombatLogEntry::Variables actorLogVariables(
    const LocalizationManager& localization,
    const CombatLogEntry& entry
) {
    CombatLogEntry::Variables variables = entry.variables;
    variables["actor"] = localizedTextIdVariable(localization, entry, "actor_text_id", "actor");
    return variables;
}

std::string statusName(
    const LocalizationManager& localization,
    const StatusDatabase& statuses,
    const std::string& statusIdText
) {
    const StatusId statusId(statusIdText);
    if (!statuses.contains(statusId)) {
        return statusIdText;
    }

    const StatusDefinition& definition = statuses.get(statusId);
    return localizedOrRaw(localization, definition.nameTextId.value, statusIdText);
}

std::string statusDescription(
    const LocalizationManager& localization,
    const StatusDatabase& statuses,
    const std::string& statusIdText
) {
    const StatusId statusId(statusIdText);
    if (!statuses.contains(statusId)) {
        return localizedOrRaw(localization, "inspect.status.unknown", statusIdText);
    }

    const StatusDefinition& definition = statuses.get(statusId);
    return localizedOrRaw(localization, definition.descriptionTextId.value, statusIdText);
}

std::string statusTypeLabel(
    const LocalizationManager& localization,
    const StatusDefinition& definition
) {
    return localizedOrRaw(localization, "status.type." + toString(definition.type), toString(definition.type));
}

std::string statusDurationLabel(
    const LocalizationManager& localization,
    const StatusDefinition& definition
) {
    return localizedOrRaw(localization, "status.duration." + toString(definition.durationRule), toString(definition.durationRule));
}

std::string statusRuntimeText(
    const LocalizationManager& localization,
    const std::string& statusIdText,
    const int amount
) {
    if (statusIdText != "poison" || amount <= 0) {
        return {};
    }

    return localizedFormat(
        localization,
        "inspect.status.poison_runtime",
        {
            {"amount", std::to_string(amount)},
            {"damage", std::to_string(amount)},
            {"remaining", std::to_string(std::max(0, amount - 1))}
        }
    );
}

std::string droneActionText(
    const LocalizationManager& localization,
    const CombatLogEntry& entry
) {
    const std::string actionTextId = variableOrFallback(entry, "action_text_id");
    if (!actionTextId.empty() && localization.hasText(TextId(actionTextId))) {
        return localization.get(TextId(actionTextId));
    }

    const std::string action = variableOrFallback(entry, "action");
    return localizedFormat(
        localization,
        "combat.log.drone_action",
        {{"action", action.empty() ? localized(localization, "combat.log.drone_action.unknown") : action}}
    );
}


std::string droneName(
    const LocalizationManager& localization,
    const DroneDatabase& drones,
    const std::string& droneType
) {
    const DroneId droneId(droneType);
    if (!drones.contains(droneId)) {
        return droneType;
    }

    const DroneDefinition& definition = drones.get(droneId);
    return localizedOrRaw(localization, definition.nameTextId.value, droneType);
}

std::string cardName(
    const LocalizationManager& localization,
    const CardDatabase& cards,
    const std::string& cardIdText
) {
    const CardId cardId(cardIdText);
    if (!cards.contains(cardId)) {
        return cardIdText;
    }

    const CardDefinition& definition = cards.get(cardId);
    return localizedOrRaw(localization, definition.nameTextId.value, cardIdText);
}

std::string enemyActionName(
    const LocalizationManager& localization,
    const std::string& actionId
) {
    const std::string textId = "enemy.action." + actionId + ".name";
    return localizedOrRaw(localization, textId, actionId);
}

std::string cardPlayFailureText(
    const LocalizationManager& localization,
    const std::string& reason
) {
    if (equalsWords(reason, {"No", "player", "actor"})) {
        return localized(localization, "ui.card_unplayable.no_player_actor");
    }
    if (equalsWords(reason, {"Not", "player", "turn"})) {
        return localized(localization, "ui.card_unplayable.not_player_turn");
    }
    if (equalsWords(reason, {"Card", "is", "not", "in", "hand"})) {
        return localized(localization, "ui.card_unplayable.card_not_in_hand");
    }
    if (equalsWords(reason, {"Invalid", "card", "source"})) {
        return localized(localization, "ui.card_unplayable.invalid_source");
    }
    if (equalsWords(reason, {"Card", "source", "is", "defeated"})) {
        return localized(localization, "ui.card_unplayable.source_defeated");
    }
    if (equalsWords(reason, {"Not", "this", "actor's", "turn"})) {
        return localized(localization, "ui.card_unplayable.wrong_actor_turn_no_active");
    }
    if (equalsWords(reason, {"Wrong", "actor", "for", "card"})) {
        return localized(localization, "ui.card_unplayable.wrong_actor_for_card");
    }
    if (equalsWords(reason, {"Not", "enough", "energy"})) {
        return localized(localization, "ui.card_unplayable.not_enough_energy");
    }
    if (equalsWords(reason, {"Card", "is", "unplayable"})) {
        return localized(localization, "ui.card_unplayable.unplayable_keyword");
    }

    return reason;
}

std::string localizeLogEntry(
    const LocalizationManager& localization,
    const StatusDatabase& statuses,
    const DroneDatabase& drones,
    const CardDatabase& cards,
    const CombatLogEntry& entry
) {
    switch (entry.type) {
        case CombatLogEntryType::Text:
            return entry.text;

        case CombatLogEntryType::CombatStarted:
            return localized(localization, "combat.log.started");

        case CombatLogEntryType::CombatWon:
            return localized(localization, "combat.log.won");

        case CombatLogEntryType::CombatLost:
            return localized(localization, "combat.log.lost");

        case CombatLogEntryType::PlayerTurnStarted:
            return localizedFormat(localization, "combat.log.player_turn_started", entry.variables);

        case CombatLogEntryType::PlayerTurnEnded:
            return localized(localization, "combat.log.player_turn_ended");

        case CombatLogEntryType::EnemyTurnStarted:
            return localized(localization, "combat.log.enemy_turn_started");

        case CombatLogEntryType::EnemyTurnEnded:
            return localized(localization, "combat.log.enemy_turn_ended");

        case CombatLogEntryType::ActivePlayerActor:
            return localizedFormat(localization, "combat.log.active_player_actor", entry.variables);

        case CombatLogEntryType::CardPlayed: {
            CombatLogEntry::Variables variables = entry.variables;
            variables["card"] = cardName(localization, cards, variableOrFallback(entry, "card"));
            return localizedFormat(localization, "combat.log.played_card", variables);
        }

        case CombatLogEntryType::CannotPlayCard: {
            CombatLogEntry::Variables variables = entry.variables;
            variables["reason"] = cardPlayFailureText(localization, variableOrFallback(entry, "reason"));
            return localizedFormat(localization, "combat.log.cannot_play_card", variables);
        }

        case CombatLogEntryType::EnemyAction: {
            CombatLogEntry::Variables variables = entry.variables;
            variables["action"] = enemyActionName(localization, variableOrFallback(entry, "action"));
            return localizedFormat(localization, "combat.log.enemy_action", variables);
        }

        case CombatLogEntryType::BossPhaseChanged: {
            CombatLogEntry::Variables variables = entry.variables;
            variables["boss"] = localizedTextIdVariable(localization, entry, "boss_text_id", "boss");
            variables["phase"] = localizedTextIdVariable(localization, entry, "phase_text_id", "phase");
            return localizedFormat(localization, "combat.log.boss_phase_changed", variables);
        }

        case CombatLogEntryType::BossArenaEffect: {
            CombatLogEntry::Variables variables = entry.variables;
            variables["boss"] = localizedTextIdVariable(localization, entry, "boss_text_id", "boss");
            variables["phase"] = localizedTextIdVariable(localization, entry, "phase_text_id", "phase");
            return localizedFormat(localization, "combat.log.boss_arena_effect", variables);
        }

        case CombatLogEntryType::EnemySummoned: {
            CombatLogEntry::Variables variables = entry.variables;
            variables["enemy"] = localizedTextIdVariable(localization, entry, "enemy_text_id", "enemy");
            return localizedFormat(localization, "combat.log.enemy_summoned", variables);
        }

        case CombatLogEntryType::DamageDealt: {
            CombatLogEntry::Variables variables = entry.variables;
            variables["source"] = localizedEntityVariable(localization, entry, "source");
            variables["target"] = localizedEntityVariable(localization, entry, "target");
            return localizedFormat(localization, "combat.log.damage_compact", variables);
        }

        case CombatLogEntryType::BlockGained: {
            CombatLogEntry::Variables variables = entry.variables;
            variables["target"] = localizedEntityVariable(localization, entry, "target");
            if (!variables.contains("modified")) {
                variables["modified"] = variableOrFallback(entry, "amount", "0");
            }
            return localizedFormat(localization, "combat.log.block_compact", variables);
        }

        case CombatLogEntryType::Heal: {
            CombatLogEntry::Variables variables = entry.variables;
            variables["target"] = localizedEntityVariable(localization, entry, "target");
            return localizedFormat(localization, "combat.log.heal_compact", variables);
        }

        case CombatLogEntryType::DrawCards:
            return localizedFormat(localization, "combat.log.draw_cards", entry.variables);

        case CombatLogEntryType::DiscardCards:
            return localizedFormat(localization, "combat.log.discard_cards", entry.variables);

        case CombatLogEntryType::GainEnergy:
            return localizedFormat(localization, "combat.log.gain_energy", entry.variables);

        case CombatLogEntryType::LoseEnergy:
            return localizedFormat(localization, "combat.log.lose_energy", entry.variables);

        case CombatLogEntryType::LoseHp:
            return localizedFormat(localization, "combat.log.lose_hp", entry.variables);

        case CombatLogEntryType::StatusApplied: {
            CombatLogEntry::Variables variables = entry.variables;
            variables["status"] = statusName(localization, statuses, variableOrFallback(entry, "status"));
            variables["target"] = localizedEntityVariable(localization, entry, "target");
            const std::string source = variableOrFallback(entry, "source_text_id");
            if (!source.empty()) {
                variables["source"] = localizedEntityVariable(localization, entry, "source");
                return localizedFormat(localization, "combat.log.status_applied_from", variables);
            }
            return localizedFormat(localization, "combat.log.status_applied", variables);
        }

        case CombatLogEntryType::PoisonDamage: {
            CombatLogEntry::Variables variables = entry.variables;
            variables["target"] = localizedTextIdVariable(localization, entry, "target_text_id", "target");
            return localizedFormat(localization, "combat.log.poison_damage", variables);
        }

        case CombatLogEntryType::BurnDamage: {
            CombatLogEntry::Variables variables = entry.variables;
            variables["target"] = localizedTextIdVariable(localization, entry, "target_text_id", "target");
            return localizedFormat(localization, "combat.log.burn_damage", variables);
        }

        case CombatLogEntryType::GainStress: {
            CombatLogEntry::Variables variables = entry.variables;
            variables["target"] = localizedEntityVariable(localization, entry, "target");
            return localizedFormat(localization, "combat.log.gain_stress_compact", variables);
        }

        case CombatLogEntryType::LoseStress: {
            CombatLogEntry::Variables variables = entry.variables;
            variables["target"] = localizedEntityVariable(localization, entry, "target");
            return localizedFormat(localization, "combat.log.lose_stress_compact", variables);
        }

        case CombatLogEntryType::StressResolve:
            return localizedFormat(localization, "combat.log.stress_resolve", actorLogVariables(localization, entry));

        case CombatLogEntryType::StressBreakdown:
            return localizedFormat(localization, "combat.log.stress_breakdown", actorLogVariables(localization, entry));
        case CombatLogEntryType::StressBreakdownPrevented:
            return localizedFormat(localization, "combat.log.stress_breakdown_prevented", actorLogVariables(localization, entry));

        case CombatLogEntryType::StressCollapse:
            return localizedFormat(localization, "combat.log.stress_collapse", actorLogVariables(localization, entry));

        case CombatLogEntryType::StressBreakdownDiscard: {
            CombatLogEntry::Variables variables = actorLogVariables(localization, entry);
            variables["card"] = cardName(localization, cards, variableOrFallback(entry, "card"));
            return localizedFormat(localization, "combat.log.stress_breakdown_discard", variables);
        }

        case CombatLogEntryType::StressBreakdownEnergy:
            return localizedFormat(localization, "combat.log.stress_breakdown_energy", actorLogVariables(localization, entry));

        case CombatLogEntryType::StressBreakdownStatusCards:
            return localizedFormat(localization, "combat.log.stress_breakdown_status_cards", actorLogVariables(localization, entry));

        case CombatLogEntryType::StressBreakdownCost:
            return localizedFormat(localization, "combat.log.stress_breakdown_cost", actorLogVariables(localization, entry));

        case CombatLogEntryType::StressBreakdownForcedCard: {
            CombatLogEntry::Variables variables = actorLogVariables(localization, entry);
            variables["card"] = cardName(localization, cards, variableOrFallback(entry, "card"));
            return localizedFormat(localization, "combat.log.stress_breakdown_forced_card", variables);
        }

        case CombatLogEntryType::MonkStanceShiftReward:
            return localizedFormat(localization, "combat.log.monk_stance_shift_reward", entry.variables);

        case CombatLogEntryType::DroneSummoned: {
            CombatLogEntry::Variables variables = entry.variables;
            variables["drone"] = droneName(localization, drones, variableOrFallback(entry, "drone"));
            return localizedFormat(localization, "combat.log.drone_summoned", variables);
        }

        case CombatLogEntryType::NoDrone:
            return localized(localization, "combat.log.no_drone");

        case CombatLogEntryType::DroneNoActiveAction: {
            CombatLogEntry::Variables variables = entry.variables;
            variables["drone"] = droneName(localization, drones, variableOrFallback(entry, "drone"));
            return localizedFormat(localization, "combat.log.drone_no_active_action", variables);
        }

        case CombatLogEntryType::DroneNoReadyAction:
            return localized(localization, "combat.log.drone_no_ready_action");

        case CombatLogEntryType::DroneAction:
            return droneActionText(localization, entry);

        case CombatLogEntryType::DroneConsumed: {
            CombatLogEntry::Variables variables = entry.variables;
            variables["drone"] = droneName(localization, drones, variableOrFallback(entry, "drone"));
            return localizedFormat(localization, "combat.log.drone_consumed", variables);
        }

        case CombatLogEntryType::UsedConsumable:
            return localizedFormat(localization, "combat.log.used_consumable", entry.variables);

        case CombatLogEntryType::RelicTriggered:
            return localizedFormat(localization, "combat.log.relic_triggered", entry.variables);

        case CombatLogEntryType::SadistHurtsMasochist:
            return localized(localization, "combat.log.sadist_hurts_masochist");

        case CombatLogEntryType::MasochistPainBonus:
            return localized(localization, "combat.log.masochist_pain_bonus");

        case CombatLogEntryType::EffectNotImplemented:
            return localizedFormat(localization, "combat.log.effect_not_implemented", entry.variables);
    }

    return entry.text;
}

bool isCompactJournalEntry(const CombatLogEntryType type) {
    switch (type) {
        case CombatLogEntryType::PlayerTurnStarted:
        case CombatLogEntryType::PlayerTurnEnded:
        case CombatLogEntryType::EnemyTurnStarted:
        case CombatLogEntryType::EnemyTurnEnded:
        case CombatLogEntryType::ActivePlayerActor:
            return false;
        default:
            return true;
    }
}

CombatJournalTone journalToneFor(const CombatLogEntryType type) {
    switch (type) {
        case CombatLogEntryType::DamageDealt:
        case CombatLogEntryType::LoseHp:
        case CombatLogEntryType::PoisonDamage:
        case CombatLogEntryType::BurnDamage:
            return CombatJournalTone::Damage;

        case CombatLogEntryType::BlockGained:
        case CombatLogEntryType::Heal:
            return CombatJournalTone::Defense;

        case CombatLogEntryType::StatusApplied:
            return CombatJournalTone::Status;

        case CombatLogEntryType::GainStress:
        case CombatLogEntryType::LoseStress:
        case CombatLogEntryType::StressResolve:
        case CombatLogEntryType::StressBreakdown:
        case CombatLogEntryType::StressBreakdownPrevented:
        case CombatLogEntryType::StressCollapse:
        case CombatLogEntryType::StressBreakdownDiscard:
        case CombatLogEntryType::StressBreakdownEnergy:
        case CombatLogEntryType::StressBreakdownStatusCards:
        case CombatLogEntryType::StressBreakdownCost:
        case CombatLogEntryType::StressBreakdownForcedCard:
            return CombatJournalTone::Stress;

        case CombatLogEntryType::DrawCards:
        case CombatLogEntryType::DiscardCards:
        case CombatLogEntryType::GainEnergy:
        case CombatLogEntryType::LoseEnergy:
        case CombatLogEntryType::UsedConsumable:
        case CombatLogEntryType::DroneSummoned:
        case CombatLogEntryType::DroneAction:
        case CombatLogEntryType::DroneConsumed:
            return CombatJournalTone::Resource;

        case CombatLogEntryType::CombatWon:
        case CombatLogEntryType::CombatLost:
        case CombatLogEntryType::BossPhaseChanged:
        case CombatLogEntryType::BossArenaEffect:
        case CombatLogEntryType::EnemySummoned:
        case CombatLogEntryType::RelicTriggered:
            return CombatJournalTone::Important;

        default:
            return CombatJournalTone::Neutral;
    }
}

std::string journalDetail(
    const LocalizationManager& localization,
    const CardDatabase& cards,
    const CombatLogEntry& entry
) {
    CombatLogEntry::Variables variables = entry.variables;
    const std::string cardId = variableOrFallback(entry, "card");
    if (!cardId.empty()) {
        variables["card"] = cardName(localization, cards, cardId);
    }

    switch (entry.type) {
        case CombatLogEntryType::DamageDealt:
            if (variableOrFallback(entry, "modifiers").empty()) {
                return localizedFormat(localization, "combat.log.damage_breakdown", variables);
            }
            return localizedFormat(localization, "combat.log.damage_breakdown_modified", variables);

        case CombatLogEntryType::BlockGained:
            if (!variables.contains("raw") || !variables.contains("modified")) {
                return {};
            }
            if (variableOrFallback(entry, "modifiers").empty()) {
                return localizedFormat(localization, "combat.log.block_breakdown", variables);
            }
            return localizedFormat(localization, "combat.log.block_breakdown_modified", variables);

        case CombatLogEntryType::GainStress:
        case CombatLogEntryType::LoseStress:
            if (variables.contains("before") && variables.contains("after")) {
                if (variableOrFallback(entry, "reason") == "card_cost" && !cardId.empty()) {
                    return localizedFormat(localization, "combat.log.stress_transition_card", variables);
                }
                return localizedFormat(localization, "combat.log.stress_transition", variables);
            }
            return {};

        case CombatLogEntryType::Heal:
            if (!cardId.empty()) {
                return localizedFormat(localization, "combat.log.caused_by_card", variables);
            }
            return {};

        default:
            return {};
    }
}
}

CombatViewModelBuilder::CombatViewModelBuilder(
    const LocalizationManager& localization,
    const StatusDatabase& statusDatabase,
    const DroneDatabase& droneDatabase,
    const CardDatabase& cardDatabase,
    const EnemyDatabase& enemyDatabase,
    const CardViewModelBuilder& cardViewModelBuilder
)
    : localization_(localization),
      statusDatabase_(statusDatabase),
      droneDatabase_(droneDatabase),
      cardDatabase_(cardDatabase),
      enemyDatabase_(enemyDatabase),
      cardViewModelBuilder_(cardViewModelBuilder) {}

CombatViewModel CombatViewModelBuilder::build(
    const CombatState& state,
    const EntityId source,
    const std::optional<EntityId> previewTarget
) const {
    return build(
        state,
        source,
        previewTarget,
        [source](const CardInstance&) {
            return source;
        }
    );
}

CombatViewModel CombatViewModelBuilder::build(
    const CombatState& state,
    const EntityId source,
    const std::optional<EntityId> previewTarget,
    const std::function<EntityId(const CardInstance&)>& sourceForCard
) const {
    (void)source;

    CombatViewModel model;
    model.phase = state.phase;
    model.turn = state.turn;
    model.energy = state.resources.energy();
    model.maxEnergy = state.resources.maxEnergy();
    model.drawPileSize = static_cast<int>(state.deck.drawPile.size());
    model.discardPileSize = static_cast<int>(state.deck.discardPile.size());
    model.exhaustPileSize = static_cast<int>(state.deck.exhaustPile.size());
    model.canEndTurn = state.phase == CombatPhase::PlayerTurn;
    model.turnLabel = localized(localization_, "ui.turn");
    model.phaseText = phaseLabel(localization_, state.phase);
    model.energyLabel = localized(localization_, "ui.energy");
    model.totalEnergyLabel = localized(localization_, "ui.total_energy");
    model.drawPileLabel = localized(localization_, "ui.draw_pile");
    model.discardPileLabel = localized(localization_, "ui.discard_pile");
    model.exhaustPileLabel = localized(localization_, "ui.exhaust_pile");
    model.endTurnLabel = localized(localization_, "ui.end_turn");
    if (state.useSequentialPlayerTurns) {
        if (const CombatEntity* active = state.activePlayer()) {
            model.endTurnLabel = localizedFormat(localization_, "ui.end_actor_turn", {
                {"actor", localizedOrRaw(localization_, active->nameTextId.value, active->definitionId)}
            });
        }
    }
    model.emptyLabel = localized(localization_, "ui.empty");
    model.droneSlotsLabel = localized(localization_, "ui.drone_slots");
    model.keyboardHintLabel = localized(localization_, "ui.combat_keyboard_hint");

    model.players.reserve(state.players.size());
    for (const CombatEntity& player : state.players) {
        PlayerViewModel playerModel;
        playerModel.entityId = player.id;
        playerModel.definitionId = player.definitionId;
        playerModel.name = localization_.get(player.nameTextId);
        playerModel.currentHp = player.health.current();
        playerModel.maxHp = player.health.maximum();
        playerModel.currentEnergy = state.resources.energyFor(player.id);
        playerModel.maxEnergy = state.resources.maxEnergyFor(player.id);
        playerModel.block = player.block;
        playerModel.stress = player.stress;
        playerModel.maxStress = player.maxStress;
        playerModel.blockLabel = localized(localization_, "ui.block");
        playerModel.stressLabel = localized(localization_, "ui.stress");
        playerModel.relicsLabel = localized(localization_, "ui.actor_relics");

        const StressRules::StressBand stressBand = StressRules::bandForStress(player.stress);
        playerModel.stressBandIndex = static_cast<int>(stressBand);
        playerModel.stressBandName = localization_.get(TextId(
            std::string("ui.stress_band.") + StressRules::bandLocalizationSuffix(stressBand) + ".name"
        ));

        if (StressPsychopathRules::appliesTo(player.definitionId)) {
            const bool hasBreakdown = StressRules::hasTrait(player, StressRules::BreakdownTraitId);
            const bool hasResolve = StressRules::hasTrait(player, StressRules::ResolveTraitId);
            const StressPsychopathRules::StressBandEffects stressEffects = StressPsychopathRules::effectsFor(
                player.stress,
                hasBreakdown,
                hasResolve
            );

            playerModel.stressPowerDamageBonus = stressEffects.attackDamageBonus;
            playerModel.stressPowerEnergyBonus = stressEffects.startTurnEnergyBonus;
            playerModel.stressPowerDiscardCount = stressEffects.startTurnDiscardCount;
            playerModel.stressBreakdownSeverity = stressEffects.breakdownSeverity;
            playerModel.stressPowerNextThreshold = StressPsychopathRules::nextDamageBonusThreshold(player.stress);
            playerModel.stressPowerLabel = localized(localization_, "ui.lost_psychopath_stress_power");
            playerModel.stressPowerDescription = localizedFormat(localization_, "ui.lost_psychopath_stress_power.value", {
                {"bonus", std::to_string(playerModel.stressPowerDamageBonus)},
                {"next", playerModel.stressPowerNextThreshold > 0 ? std::to_string(playerModel.stressPowerNextThreshold) : "-"}
            });

            if (stressEffects.startTurnEnergyBonus > 0 && stressEffects.breakdownSeverity > 0) {
                playerModel.stressBandRiskDescription = localizedFormat(
                    localization_,
                    "ui.lost_psychopath_stress_risk.energy_and_breakdown",
                    {
                        {"energy", std::to_string(stressEffects.startTurnEnergyBonus)},
                        {"severity", std::to_string(stressEffects.breakdownSeverity)}
                    }
                );
            } else if (stressEffects.breakdownSeverity > 0) {
                playerModel.stressBandRiskDescription = localizedFormat(
                    localization_,
                    "ui.lost_psychopath_stress_risk.breakdown",
                    {{"severity", std::to_string(stressEffects.breakdownSeverity)}}
                );
            } else if (stressEffects.startTurnEnergyBonus > 0 && stressEffects.startTurnDiscardCount > 0) {
                playerModel.stressBandRiskDescription = localizedFormat(
                    localization_,
                    "ui.lost_psychopath_stress_risk.energy_and_discard",
                    {
                        {"energy", std::to_string(stressEffects.startTurnEnergyBonus)},
                        {"cards", std::to_string(stressEffects.startTurnDiscardCount)}
                    }
                );
            } else if (stressEffects.startTurnEnergyBonus > 0) {
                playerModel.stressBandRiskDescription = localizedFormat(
                    localization_,
                    "ui.lost_psychopath_stress_risk.energy",
                    {{"energy", std::to_string(stressEffects.startTurnEnergyBonus)}}
                );
            } else if (stressEffects.startTurnDiscardCount > 0) {
                playerModel.stressBandRiskDescription = localizedFormat(
                    localization_,
                    "ui.lost_psychopath_stress_risk.discard",
                    {{"cards", std::to_string(stressEffects.startTurnDiscardCount)}}
                );
            } else {
                playerModel.stressBandRiskDescription = localized(localization_, "ui.lost_psychopath_stress_risk.stable");
            }
        }
        playerModel.activeTurnLabel = state.useSequentialPlayerTurns
            ? localized(localization_, "ui.active_turn")
            : localized(localization_, "ui.card_source_marker");
        playerModel.activeStanceLabel = localized(localization_, "ui.active_stance");
        playerModel.stanceShiftBonusLabel = localized(localization_, "ui.stance_shift_bonus");
        fillActiveStance(playerModel, player.statuses);
        if (const std::optional<EntityId> activePlayer = state.activePlayerId()) {
            playerModel.activeTurn = *activePlayer == player.id;
        }
        playerModel.traitIds = player.traitIds;
        playerModel.statuses = buildStatuses(player.statuses, false);
        playerModel.alive = player.isAlive();
        model.players.push_back(std::move(playerModel));
    }

    if (!state.players.empty()) {
        const CombatEntity& player = state.players.front();
        model.playerCurrentHp = player.health.current();
        model.playerMaxHp = player.health.maximum();
        model.playerBlock = player.block;
    }

    model.handCards.reserve(state.hand.cards().size());
    for (const CardInstance& card : state.hand.cards()) {
        model.handCards.push_back(
            cardViewModelBuilder_.build(
                state,
                card.instanceId,
                sourceForCard(card),
                previewTarget
            )
        );
    }

    int lowestLivingPlayerHp = 0;
    for (const CombatEntity& player : state.players) {
        if (!player.isAlive()) {
            continue;
        }
        if (lowestLivingPlayerHp == 0 || player.health.current() < lowestLivingPlayerHp) {
            lowestLivingPlayerHp = player.health.current();
        }
    }

    std::unordered_map<std::uint64_t, EnemyIntent> intentsByEnemy;
    intentsByEnemy.reserve(state.enemyIntents.size());
    for (const EnemyIntentState& intentState : state.enemyIntents) {
        intentsByEnemy.emplace(intentState.enemyId.value, intentState.intent);
    }

    model.enemies.reserve(state.enemies.size());
    for (std::size_t enemyIndex = 0; enemyIndex < state.enemies.size(); ++enemyIndex) {
        const CombatEntity& enemy = state.enemies[enemyIndex];
        EnemyViewModel enemyModel;
        enemyModel.formationLabel = std::to_string(enemyIndex + 1) + "/" + std::to_string(state.enemies.size());
        enemyModel.defeatedLabel = localized(localization_, "ui.enemy.defeated");
        enemyModel.entityId = enemy.id;
        enemyModel.name = localization_.get(enemy.nameTextId);
        if (enemyDatabase_.contains(EnemyId(enemy.definitionId))) {
            const EnemyDefinition& definition = enemyDatabase_.get(EnemyId(enemy.definitionId));
            if (const EnemyPhaseDefinition* phase = BossPhaseRules::activePhase(state, definition, enemy)) {
                enemyModel.phaseName = localization_.get(phase->nameTextId);
            }
        }
        enemyModel.currentHp = enemy.health.current();
        enemyModel.maxHp = enemy.health.maximum();
        enemyModel.block = enemy.block;
        enemyModel.blockLabel = localized(localization_, "ui.block");
        enemyModel.statuses = buildStatuses(enemy.statuses);
        enemyModel.alive = enemy.isAlive();

        const auto intentIterator = intentsByEnemy.find(enemy.id.value);
        if (intentIterator != intentsByEnemy.end()) {
            enemyModel.intent = intentIterator->second;
            enemyModel.intentText = intentLabel(localization_, enemyModel.intent);
            enemyModel.intentDetailText = intentDetailText(localization_, statusDatabase_, enemyModel.intent);

            const EnemyIntentPresentation presentation = summarizeEnemyIntent(enemyModel.intent);
            enemyModel.intentAffectsMultipleTargets = presentation.affectsMultipleTargets;
            enemyModel.intentDangerLevel = intentDangerLevel(enemyModel.intent, lowestLivingPlayerHp);

            if (enemyModel.intent.type == EnemyIntentType::Attack && enemyModel.alive) {
                const int hitCount = std::max(1, enemyModel.intent.hitCount);
                model.incomingDamageMin += std::max(0, enemyModel.intent.valueMin) * hitCount;
                model.incomingDamageMax += std::max(0, enemyModel.intent.valueMax) * hitCount;
                ++model.attackingEnemyCount;
                if (presentation.affectsAllPlayers) {
                    ++model.partyWideThreatCount;
                }
            }

            if (presentation.affectsAllPlayers && presentation.affectsEnemyTeam) {
                enemyModel.intentScopeLabel = localized(localization_, "ui.intent_scope.party_and_team");
            } else if (presentation.affectsAllPlayers) {
                enemyModel.intentScopeLabel = localized(localization_, "ui.intent_scope.party");
            } else if (presentation.affectsEnemyTeam) {
                enemyModel.intentScopeLabel = localized(localization_, "ui.intent_scope.enemy_team");
            }
        } else {
            enemyModel.intent.type = EnemyIntentType::Unknown;
            enemyModel.intentText = enemyModel.alive ? "..." : "";
            enemyModel.intentDetailText = enemyModel.alive ? intentDetailText(localization_, statusDatabase_, enemyModel.intent) : "";
        }

        model.enemies.push_back(std::move(enemyModel));
    }

    if (model.attackingEnemyCount > 0) {
        model.incomingDamageLabel = localizedFormat(
            localization_,
            "ui.threat.incoming_damage",
            {{"damage", numericRangeText(model.incomingDamageMin, model.incomingDamageMax)}}
        );
    } else {
        model.incomingDamageLabel = localized(localization_, "ui.threat.no_attacks");
    }

    if (model.partyWideThreatCount > 0) {
        model.partyWideThreatLabel = localizedFormat(
            localization_,
            "ui.threat.party_wide",
            {{"count", std::to_string(model.partyWideThreatCount)}}
        );
    }

    model.recentJournalEntries = recentJournalEntries(state, 7);
    return model;
}


void CombatViewModelBuilder::fillActiveStance(
    PlayerViewModel& model,
    const StatusContainer& statuses
) const {
    for (const char* stanceId : {stanceFlameStatusId, stanceAshStatusId, stanceSmokeStatusId}) {
        if (!statuses.has(stanceId) || !statusDatabase_.contains(StatusId(stanceId))) {
            continue;
        }

        const StatusDefinition& definition = statusDatabase_.get(StatusId(stanceId));
        model.activeStanceName = localization_.get(definition.nameTextId);
        model.activeStanceDescription = localization_.get(definition.descriptionTextId);
        return;
    }
}

std::vector<StatusViewModel> CombatViewModelBuilder::buildStatuses(
    const StatusContainer& statuses,
    const bool includeStances
) const {
    std::vector<StatusViewModel> result;

    const std::vector<std::pair<std::string, int>> entries = statuses.all();
    result.reserve(entries.size());

    for (const auto& [statusId, amount] : entries) {
        if (!includeStances && isStanceStatus(statusId)) {
            continue;
        }

        StatusViewModel model;
        model.id = statusId;
        model.amount = amount;

        if (statusDatabase_.contains(StatusId(statusId))) {
            const StatusDefinition& definition = statusDatabase_.get(StatusId(statusId));
            model.name = localization_.get(definition.nameTextId);
            model.description = statusDescription(localization_, statusDatabase_, statusId);
            model.typeLabel = statusTypeLabel(localization_, definition);
            model.durationLabel = statusDurationLabel(localization_, definition);
            model.runtimeText = statusRuntimeText(localization_, statusId, amount);
            model.buff = definition.type == StatusType::Buff;
            model.debuff = definition.type == StatusType::Debuff;
        } else {
            model.name = statusId;
            model.description = localizedOrRaw(localization_, "inspect.status.unknown", statusId);
            model.typeLabel.clear();
            model.durationLabel.clear();
            model.runtimeText.clear();
            model.buff = false;
            model.debuff = false;
        }

        result.push_back(std::move(model));
    }

    return result;
}

std::vector<CombatJournalEntryViewModel> CombatViewModelBuilder::recentJournalEntries(
    const CombatState& state,
    const std::size_t maxCount
) const {
    const std::vector<CombatLogEntry>& entries = state.log.entries();
    std::vector<CombatJournalEntryViewModel> reversed;
    reversed.reserve(std::min(maxCount, entries.size()));

    for (auto iterator = entries.rbegin(); iterator != entries.rend() && reversed.size() < maxCount; ++iterator) {
        if (!isCompactJournalEntry(iterator->type)) {
            continue;
        }

        const std::string text = localizeLogEntry(
            localization_,
            statusDatabase_,
            droneDatabase_,
            cardDatabase_,
            *iterator
        );
        if (text.empty()) {
            continue;
        }

        CombatJournalEntryViewModel model;
        model.sequence = iterator->sequence;
        model.tone = journalToneFor(iterator->type);
        model.text = text;
        model.detail = journalDetail(localization_, cardDatabase_, *iterator);
        reversed.push_back(std::move(model));
    }

    std::reverse(reversed.begin(), reversed.end());
    return reversed;
}
