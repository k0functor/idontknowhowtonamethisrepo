#include "CardUpgrade.hpp"

#include "cards/CardType.hpp"
#include "effects/EffectType.hpp"
#include "localization/LocalizationManager.hpp"
#include "localization/TextId.hpp"

#include <algorithm>
#include <string>
#include <utility>

bool CardUpgradeDefinition::empty() const {
    return !nameTextId.has_value() &&
        !descriptionTextId.has_value() &&
        !energyCost.has_value() &&
        !goldCost.has_value() &&
        !keywords.has_value() &&
        !diceCorruption.has_value() &&
        !effects.has_value();
}

namespace {
bool canImproveValue(const EffectType type) {
    switch (type) {
        case EffectType::Damage:
        case EffectType::Block:
        case EffectType::Heal:
        case EffectType::DrawCards:
        case EffectType::ApplyStatus:
        case EffectType::GainEnergy:
        case EffectType::LoseStress:
            return true;
        case EffectType::DiscardCards:
        case EffectType::GainStress:
        case EffectType::LoseEnergy:
        case EffectType::LoseHp:
        case EffectType::EnterStance:
        case EffectType::SummonDrone:
        case EffectType::UseDrone:
            return false;
    }

    return false;
}

int improvementFor(const EffectType type) {
    switch (type) {
        case EffectType::Damage:
        case EffectType::Block:
        case EffectType::Heal:
            return 2;
        case EffectType::ApplyStatus:
        case EffectType::DrawCards:
        case EffectType::GainEnergy:
        case EffectType::LoseStress:
            return 1;
        default:
            return 0;
    }
}

EffectValue improvedValue(const EffectValue& value, const int delta) {
    if (value.isFixed()) {
        return EffectValue::fixed(value.fixedAmount() + delta);
    }

    DiceExpression expression = value.diceExpression();
    expression.bonus += delta;
    return EffectValue::dice(expression);
}

bool improveFirstEffect(std::vector<EffectDefinition>& effects) {
    for (EffectDefinition& effect : effects) {
        if (!canImproveValue(effect.type)) {
            continue;
        }

        const int delta = improvementFor(effect.type);
        if (delta <= 0) {
            continue;
        }

        effect.value = improvedValue(effect.value, delta);
        return true;
    }

    return false;
}

CardDefinition automaticallyUpgraded(CardDefinition result) {
    if (result.energyCost > 0) {
        --result.energyCost;
        return result;
    }

    improveFirstEffect(result.effects);
    return result;
}
}

namespace CardUpgrade {
bool isUpgradable(const CardDefinition& definition) {
    return definition.type != CardType::Status && definition.type != CardType::Curse;
}

CardDefinition upgradedDefinition(const CardDefinition& definition) {
    CardDefinition result = definition;

    if (!definition.upgrade.empty()) {
        if (definition.upgrade.nameTextId.has_value()) {
            result.nameTextId = *definition.upgrade.nameTextId;
        }
        if (definition.upgrade.descriptionTextId.has_value()) {
            result.descriptionTextId = *definition.upgrade.descriptionTextId;
        }
        if (definition.upgrade.energyCost.has_value()) {
            result.energyCost = *definition.upgrade.energyCost;
        }
        if (definition.upgrade.goldCost.has_value()) {
            result.goldCost = *definition.upgrade.goldCost;
        }
        if (definition.upgrade.keywords.has_value()) {
            result.keywords = *definition.upgrade.keywords;
        }
        if (definition.upgrade.diceCorruption.has_value()) {
            result.diceCorruption = *definition.upgrade.diceCorruption;
        }
        if (definition.upgrade.effects.has_value()) {
            result.effects = *definition.upgrade.effects;
        }
        return result;
    }

    return automaticallyUpgraded(std::move(result));
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
