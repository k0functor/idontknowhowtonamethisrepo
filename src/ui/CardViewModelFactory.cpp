#include "CardViewModelFactory.hpp"

#include "cards/CardDescriptionFormatter.hpp"
#include "cards/CardUpgrade.hpp"

namespace CardViewModelFactory {
CardViewModel buildStatic(
    const CardDefinition& definition,
    const LocalizationManager& localization,
    const CardInstanceId instanceId,
    const bool upgraded,
    const bool selected,
    const bool playable
) {
    const CardDefinition effectiveDefinition = CardUpgrade::effectiveDefinition(definition, upgraded);
    const CardDescriptionFormatter descriptionFormatter(localization);

    CardViewModel model;
    model.instanceId = instanceId;
    model.name = localization.get(effectiveDefinition.nameTextId) + (upgraded ? "+" : "");
    model.description = descriptionFormatter.formatStaticDescription(effectiveDefinition);
    model.energyCost = effectiveDefinition.energyCost;
    model.type = effectiveDefinition.type;
    model.rarity = effectiveDefinition.rarity;
    model.playable = playable;
    model.selected = selected;
    model.upgraded = upgraded;
    return model;
}
}
