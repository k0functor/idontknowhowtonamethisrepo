#include "CardViewModelBuilder.hpp"

#include "cards/CardDefinition.hpp"
#include "localization/TextFormatter.hpp"

#include <string>

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

    for (const EffectPreview& effectPreview : preview.effects) {
        if (effectPreview.type == EffectType::Damage) {
            if (effectPreview.damage.has_value()) {
                variables["damage"] = rangeToString(
                    effectPreview.damage->modifiedMin,
                    effectPreview.damage->modifiedMax
                );
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
            variables["block"] = effectPreview.value.toDisplayString();
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
