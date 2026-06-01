#pragma once

#include "cards/CardDefinition.hpp"
#include "combat/CombatState.hpp"
#include "effects/EffectDefinition.hpp"
#include "effects/EffectValue.hpp"
#include "entities/EntityId.hpp"
#include "localization/LocalizationManager.hpp"
#include "localization/TextFormatter.hpp"
#include "preview/CardPreview.hpp"

#include <optional>
#include <string>

class CardDescriptionFormatter {
public:
    explicit CardDescriptionFormatter(const LocalizationManager& localization);

    std::string formatStaticDescription(const CardDefinition& card) const;

    std::string formatCombatDescription(
        const CardDefinition& card,
        const CardPreview& preview,
        const CombatState& state,
        EntityId source,
        std::optional<EntityId> target
    ) const;

private:
    TextFormatter::Variables defaultVariables() const;

    void fillVariablesFromStaticEffect(
        TextFormatter::Variables& variables,
        const EffectDefinition& effect
    ) const;

    void fillVariablesFromPreviewEffect(
        TextFormatter::Variables& variables,
        const CardDefinition& card,
        const CardPreview& preview,
        const CombatState& state,
        EntityId source,
        std::optional<EntityId> target
    ) const;

    static std::string effectValueText(const EffectValue& value);
    static std::string rangeToString(int minimum, int maximum);

    std::string repeatSuffix(int repeatCount) const;

    static std::string formulaSuffixForDamage(
        const CombatState& state,
        EntityId source,
        std::optional<EntityId> target,
        const EffectDefinition& effect
    );

    static std::string formulaSuffixForBlock(
        const CombatState& state,
        EntityId source,
        const EffectDefinition& effect
    );

private:
    const LocalizationManager& localization_;
};
