#include "CardViewModelBuilder.hpp"

#include "cards/CardDefinition.hpp"
#include "dice/DiceExpression.hpp"
#include "effects/EffectDefinition.hpp"
#include "effects/EffectValue.hpp"
#include "localization/TextFormatter.hpp"

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

CardViewModelBuilder::CardViewModelBuilder(
    const CardDatabase& cardDatabase,
    const LocalizationManager& localization,
    const CardPreviewSystem& previewSystem
)
    : cardDatabase_(cardDatabase),
      localization_(localization),
      previewSystem_(previewSystem) {}

CardViewModel CardViewModelBuilder::build(
    const CombatState& state,
    const CardInstanceId cardInstanceId,
    const EntityId source,
    const std::optional<EntityId> target
) const {
    const CardInstance& instance = state.hand.get(cardInstanceId);
    const CardDefinition& definition = cardDatabase_.get(instance.definitionId);

    const CardPreview preview = previewSystem_.previewCard(
        state,
        cardInstanceId,
        source,
        target
    );

    TextFormatter::Variables variables;
    variables.emplace("damage", "?");
    variables.emplace("hp_damage", "?");
    variables.emplace("block", "?");
    variables.emplace("poison", "?");
    variables.emplace("value", "?");

    for (std::size_t i = 0; i < preview.effects.size(); ++i) {
        const EffectPreview& effectPreview = preview.effects[i];
        const EffectDefinition* effectDefinition = i < definition.effects.size()
            ? &definition.effects[i]
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
            } else {
                variables["damage"] = effectPreview.value.toDisplayString();
                variables["hp_damage"] = effectPreview.value.toDisplayString();
            }
        }

        if (effectPreview.type == EffectType::Block) {
            std::string blockText = effectPreview.value.toDisplayString();

            if (effectDefinition != nullptr) {
                blockText += formulaSuffixForBlock(state, source, *effectDefinition);
            }

            variables["block"] = blockText;
        }

        if (effectPreview.type == EffectType::ApplyStatus) {
            variables["value"] = effectPreview.value.toDisplayString();

            if (effectPreview.statusId.has_value()) {
                variables[*effectPreview.statusId] = effectPreview.value.toDisplayString();
            }
        }
    }

    CardViewModel model;
    model.instanceId = cardInstanceId;
    model.name = localization_.get(definition.nameTextId);
    model.description = localization_.format(definition.descriptionTextId, variables);
    model.energyCost = preview.energyCost;
    model.type = definition.type;
    model.rarity = definition.rarity;
    model.playable = preview.playable;
    return model;
}

std::string CardViewModelBuilder::rangeToString(const int minimum, const int maximum) {
    if (minimum == maximum) {
        return std::to_string(minimum);
    }

    return std::to_string(minimum) + "-" + std::to_string(maximum);
}

std::string CardViewModelBuilder::formulaSuffixForDamage(
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

std::string CardViewModelBuilder::formulaSuffixForBlock(
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
