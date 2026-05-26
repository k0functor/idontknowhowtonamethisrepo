#pragma once

#include "cards/CardDefinition.hpp"
#include "inspect/InspectPanelModel.hpp"
#include "data/ContentRegistry.hpp"
#include "localization/LocalizationManager.hpp"
#include "ui/CardViewModel.hpp"
#include "ui/CombatViewModel.hpp"
#include "ui/ConsumableViewModel.hpp"
#include "ui/EnemyViewModel.hpp"
#include "ui/PlayerViewModel.hpp"

class InspectModelBuilder {
public:
    InspectModelBuilder(
        const ContentRegistry& content,
        const LocalizationManager& localization
    );

    InspectPanelModel buildEnemy(const EnemyViewModel& enemy) const;
    InspectPanelModel buildPlayer(const PlayerViewModel& player) const;
    InspectPanelModel buildConsumable(const ConsumableViewModel& consumable) const;
    InspectPanelModel buildDroneSlot(const DroneSlotViewModel& droneSlot) const;

    InspectPanelModel buildCard(
        const CardDefinition& definition,
        const CardViewModel& card
    ) const;

private:
    std::string textOrFallback(const TextId& textId, const std::string& fallback) const;
    std::string rawTextOrFallback(const std::string& textId, const std::string& fallback) const;
    std::string keywordName(CardKeyword keyword) const;
    std::string keywordDescription(CardKeyword keyword) const;
    std::string statusName(const std::string& statusId) const;
    std::string statusDescription(const std::string& statusId) const;
    std::string effectSummary(const EffectDefinition& effect) const;
    std::string targetText(EffectTarget target) const;
    std::string valueText(const EffectValue& value) const;

private:
    const ContentRegistry& content_;
    const LocalizationManager& localization_;
};
