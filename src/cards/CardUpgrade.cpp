#include "CardUpgrade.hpp"

#include "cards/CardType.hpp"
#include "localization/LocalizationManager.hpp"
#include "localization/TextId.hpp"

#include <string>

bool CardUpgradeDefinition::empty() const {
    return !nameTextId.has_value() &&
        !descriptionTextId.has_value() &&
        !energyCost.has_value() &&
        !stressCost.has_value() &&
        !goldCost.has_value() &&
        !keywords.has_value() &&
        !diceCorruption.has_value() &&
        !effects.has_value();
}

namespace {
void applyUpgradeOverride(CardDefinition& result, const CardUpgradeDefinition& upgrade) {
    if (upgrade.nameTextId.has_value()) {
        result.nameTextId = *upgrade.nameTextId;
    }
    if (upgrade.descriptionTextId.has_value()) {
        result.descriptionTextId = *upgrade.descriptionTextId;
    }
    if (upgrade.energyCost.has_value()) {
        result.energyCost = *upgrade.energyCost;
    }
    if (upgrade.stressCost.has_value()) {
        result.stressCost = *upgrade.stressCost;
    }
    if (upgrade.goldCost.has_value()) {
        result.goldCost = *upgrade.goldCost;
    }
    if (upgrade.keywords.has_value()) {
        result.keywords = *upgrade.keywords;
    }
    if (upgrade.diceCorruption.has_value()) {
        result.diceCorruption = *upgrade.diceCorruption;
    }
    if (upgrade.effects.has_value()) {
        result.effects = *upgrade.effects;
    }
}
}

namespace CardUpgrade {
bool isUpgradable(const CardDefinition& definition) {
    return definition.type != CardType::Status &&
        definition.type != CardType::Curse &&
        !definition.upgrade.empty();
}

CardDefinition upgradedDefinition(const CardDefinition& definition) {
    CardDefinition result = definition;

    if (definition.upgrade.empty()) {
        return result;
    }

    applyUpgradeOverride(result, definition.upgrade);
    return result;
}

CardDefinition effectiveDefinition(const CardDefinition& definition, const bool upgraded) {
    if (!upgraded) {
        return definition;
    }

    return upgradedDefinition(definition);
}

std::string summary(const CardDefinition& base, const CardDefinition& upgraded, const LocalizationManager& localization) {
    std::string result;

    if (base.energyCost != upgraded.energyCost) {
        result += localization.format(TextId("card.upgrade.summary.cost"), {{"before", std::to_string(base.energyCost)}, {"after", std::to_string(upgraded.energyCost)}});
    }

    if (base.stressCost != upgraded.stressCost) {
        if (!result.empty()) {
            result += "; ";
        }
        result += localization.format(
            TextId("card.upgrade.summary.stress_cost"),
            {{"before", std::to_string(base.stressCost)}, {"after", std::to_string(upgraded.stressCost)}}
        );
    }

    if (base.effects.size() != upgraded.effects.size()) {
        if (!result.empty()) {
            result += "; ";
        }
        result += localization.get(TextId("card.upgrade.summary.effects_changed"));
    } else {
        for (std::size_t i = 0; i < base.effects.size(); ++i) {
            const EffectDefinition& before = base.effects[i];
            const EffectDefinition& after = upgraded.effects[i];
            if (before.type != after.type ||
                before.target != after.target ||
                before.repeatCount != after.repeatCount ||
                before.outputAmount != after.outputAmount ||
                before.value.minimumPossibleValue() != after.value.minimumPossibleValue() ||
                before.value.maximumPossibleValue() != after.value.maximumPossibleValue()) {
                if (!result.empty()) {
                    result += "; ";
                }
                result += localization.format(TextId("card.upgrade.summary.effect_improved"), {{"index", std::to_string(i + 1)}});
            }
        }
    }

    if (result.empty()) {
        result = localization.get(TextId("card.upgrade.summary.improved_version"));
    }

    return result;
}
}
