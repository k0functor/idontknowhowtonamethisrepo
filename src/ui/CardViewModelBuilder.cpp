#include "CardViewModelBuilder.hpp"

#include "cards/CardDefinition.hpp"
#include "cards/CardDescriptionFormatter.hpp"
#include "cards/CardUpgrade.hpp"

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
    const CardDefinition definition = CardUpgrade::effectiveDefinition(cardDatabase_.get(instance.definitionId), instance.upgraded);

    const CardPreview preview = previewSystem_.previewCard(
        state,
        cardInstanceId,
        source,
        target
    );

    const CardDescriptionFormatter descriptionFormatter(localization_);

    CardViewModel model;
    model.instanceId = cardInstanceId;
    model.name = localization_.get(definition.nameTextId) + (instance.upgraded ? "+" : "");
    model.description = descriptionFormatter.formatCombatDescription(
        definition,
        preview,
        state,
        source,
        target
    );
    model.energyCost = preview.energyCost;
    model.type = definition.type;
    model.rarity = definition.rarity;
    model.playable = preview.playable;
    return model;
}
