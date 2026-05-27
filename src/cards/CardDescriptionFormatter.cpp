#include "cards/CardDescriptionFormatter.hpp"

#include "dice/DiceExpression.hpp"
#include "preview/EffectPreview.hpp"

#include <cstddef>
#include <string>

namespace {
constexpr const char* strengthStatusId = "strength";
constexpr const char* dexterityStatusId = "dexterity";

int statusStacks(const CombatState& state, const EntityId entityId, const std::string& statusId) {
    if (!state.hasEntity(entityId)) {
        return 0;
    }

    return state.entity(entityId).statuses.stacks(statusId);
}

std::string signedAdd(const int value) {
    if (value > 0) {
        return "+" + std::to_string(value);
    }

    if (value < 0) {
        return "-" + std::to_string(-value);
    }

    return "";
}
}

CardDescriptionFormatter::CardDescriptionFormatter(const LocalizationManager& localization)
    : localization_(localization) {}

std::string CardDescriptionFormatter::formatStaticDescription(const CardDefinition& card) const {
    TextFormatter::Variables variables = defaultVariables();

    for (const EffectDefinition& effect : card.effects) {
        fillVariablesFromStaticEffect(variables, effect);
    }

    return localization_.format(card.descriptionTextId, variables);
}

std::string CardDescriptionFormatter::formatCombatDescription(
    const CardDefinition& card,
    const CardPreview& preview,
    const CombatState& state,
    const EntityId source,
    const std::optional<EntityId> target
) const {
    TextFormatter::Variables variables = defaultVariables();
    fillVariablesFromPreviewEffect(variables, card, preview, state, source, target);
    return localization_.format(card.descriptionTextId, variables);
}

TextFormatter::Variables CardDescriptionFormatter::defaultVariables() const {
    TextFormatter::Variables variables;

    variables.emplace("damage", "?");
    variables.emplace("hp_damage", "?");
    variables.emplace("block", "?");
    variables.emplace("heal", "?");
    variables.emplace("draw_cards", "?");
    variables.emplace("discard_cards", "?");
    variables.emplace("energy", "?");
    variables.emplace("stress", "?");
    variables.emplace("lose_hp", "?");
    variables.emplace("poison", "?");
    variables.emplace("burn", "?");
    variables.emplace("strength", "?");
    variables.emplace("dexterity", "?");
    variables.emplace("vulnerable", "?");
    variables.emplace("weak", "?");
    variables.emplace("free_next_card", "?");
    variables.emplace("value", "?");
    variables.emplace("times", "?");

    return variables;
}

void CardDescriptionFormatter::fillVariablesFromStaticEffect(
    TextFormatter::Variables& variables,
    const EffectDefinition& effect
) const {
    const std::string value = effectValueText(effect.value);
    variables["times"] = std::to_string(effect.repeatCount);

    switch (effect.type) {
        case EffectType::Damage:
            variables["damage"] = value;
            variables["hp_damage"] = value;
            variables["value"] = value;
            break;

        case EffectType::Block:
            variables["block"] = value;
            variables["value"] = value;
            break;

        case EffectType::Heal:
            variables["heal"] = value;
            variables["value"] = value;
            break;

        case EffectType::DrawCards:
            variables["draw_cards"] = value;
            variables["value"] = value;
            break;

        case EffectType::DiscardCards:
            variables["discard_cards"] = value;
            variables["value"] = value;
            break;

        case EffectType::GainEnergy:
        case EffectType::LoseEnergy:
            variables["energy"] = value;
            variables["value"] = value;
            break;

        case EffectType::GainStress:
        case EffectType::LoseStress:
            variables["stress"] = value;
            variables["value"] = value;
            break;

        case EffectType::LoseHp:
            variables["lose_hp"] = value;
            variables["hp_damage"] = value;
            variables["value"] = value;
            break;

        case EffectType::ApplyStatus:
            variables["value"] = value;
            if (effect.statusId.has_value()) {
                variables[*effect.statusId] = value;
            }
            break;

        case EffectType::EnterStance:
        case EffectType::SummonDrone:
        case EffectType::UseDrone:
            variables["value"] = value;
            break;
    }
}

void CardDescriptionFormatter::fillVariablesFromPreviewEffect(
    TextFormatter::Variables& variables,
    const CardDefinition& card,
    const CardPreview& preview,
    const CombatState& state,
    const EntityId source,
    const std::optional<EntityId> target
) const {
    for (std::size_t i = 0; i < preview.effects.size(); ++i) {
        const EffectPreview& effectPreview = preview.effects[i];
        const EffectDefinition* effectDefinition = i < card.effects.size()
            ? &card.effects[i]
            : nullptr;

        if (effectPreview.type == EffectType::Damage) {
            if (effectPreview.damage.has_value()) {
                std::string damageText = rangeToString(
                    effectPreview.damage->modifiedMin,
                    effectPreview.damage->modifiedMax
                );

                if (effectDefinition != nullptr) {
                    damageText += formulaSuffixForDamage(
                        state,
                        source,
                        target,
                        *effectDefinition
                    );
                }

                variables["damage"] = damageText;
                variables["hp_damage"] = rangeToString(
                    effectPreview.damage->hpDamageMin,
                    effectPreview.damage->hpDamageMax
                );
                variables["value"] = damageText;
            } else {
                const std::string value = effectPreview.value.toDisplayString();
                variables["damage"] = value;
                variables["hp_damage"] = value;
                variables["value"] = value;
            }
        }

        if (effectPreview.type == EffectType::Block) {
            std::string blockText = effectPreview.value.toDisplayString();

            if (effectDefinition != nullptr) {
                blockText += formulaSuffixForBlock(state, source, *effectDefinition);
            }

            variables["block"] = blockText;
            variables["value"] = blockText;
        }

        const std::string valueText = effectPreview.value.toDisplayString();
        variables["times"] = std::to_string(effectPreview.repeatCount);

        switch (effectPreview.type) {
            case EffectType::Heal:
                variables["heal"] = valueText;
                variables["value"] = valueText;
                break;

            case EffectType::DrawCards:
                variables["draw_cards"] = valueText;
                variables["value"] = valueText;
                break;

            case EffectType::DiscardCards:
                variables["discard_cards"] = valueText;
                variables["value"] = valueText;
                break;

            case EffectType::GainEnergy:
            case EffectType::LoseEnergy:
                variables["energy"] = valueText;
                variables["value"] = valueText;
                break;

            case EffectType::GainStress:
            case EffectType::LoseStress:
                variables["stress"] = valueText;
                variables["value"] = valueText;
                break;

            case EffectType::LoseHp:
                variables["lose_hp"] = valueText;
                variables["hp_damage"] = valueText;
                variables["value"] = valueText;
                break;

            case EffectType::ApplyStatus:
                variables["value"] = valueText;
                if (effectPreview.statusId.has_value()) {
                    variables[*effectPreview.statusId] = valueText;
                }
                break;

            case EffectType::Damage:
            case EffectType::Block:
            case EffectType::EnterStance:
            case EffectType::SummonDrone:
            case EffectType::UseDrone:
                break;
        }
    }
}

std::string CardDescriptionFormatter::effectValueText(const EffectValue& value) {
    const std::string range = rangeToString(
        value.minimumPossibleValue(),
        value.maximumPossibleValue()
    );

    if (value.isDice()) {
        return range + " (" + ::toString(value.diceExpression()) + ")";
    }

    return range;
}

std::string CardDescriptionFormatter::rangeToString(const int minimum, const int maximum) {
    if (minimum == maximum) {
        return std::to_string(minimum);
    }

    return std::to_string(minimum) + "-" + std::to_string(maximum);
}

std::string CardDescriptionFormatter::formulaSuffixForDamage(
    const CombatState& state,
    const EntityId source,
    const std::optional<EntityId> target,
    const EffectDefinition& effect
) {
    (void)target;

    if (!effect.value.isDice()) {
        return "";
    }

    std::string formula = ::toString(effect.value.diceExpression());

    const int strength = statusStacks(state, source, strengthStatusId);
    if (strength != 0) {
        formula += signedAdd(strength);
    }

    return " (" + formula + ")";
}

std::string CardDescriptionFormatter::formulaSuffixForBlock(
    const CombatState& state,
    const EntityId source,
    const EffectDefinition& effect
) {
    if (!effect.value.isDice()) {
        return "";
    }

    std::string formula = ::toString(effect.value.diceExpression());

    const int dexterity = statusStacks(state, source, dexterityStatusId);
    if (dexterity != 0) {
        formula += signedAdd(dexterity);
    }

    return " (" + formula + ")";
}
