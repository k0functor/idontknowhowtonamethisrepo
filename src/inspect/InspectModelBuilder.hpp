#pragma once

#include "cards/CardDefinition.hpp"
#include "inspect/InspectPanelModel.hpp"
#include "data/ContentRegistry.hpp"
#include "localization/LocalizationManager.hpp"
#include "consumables/ConsumableDefinition.hpp"
#include "relics/RelicDefinition.hpp"
#include "drones/DroneActionDefinition.hpp"
#include "statuses/StatusDurationRule.hpp"
#include "statuses/StatusType.hpp"
#include "ui/CardViewModel.hpp"
#include "ui/CombatViewModel.hpp"
#include "ui/ConsumableViewModel.hpp"
#include "ui/EnemyViewModel.hpp"
#include "ui/PlayerViewModel.hpp"

#include <string>
#include <utility>
#include <vector>

class InspectModelBuilder {
public:
    InspectModelBuilder(
        const ContentRegistry& content,
        const LocalizationManager& localization
    );

    InspectPanelModel buildEnemy(const EnemyViewModel& enemy) const;
    InspectPanelModel buildPlayer(const PlayerViewModel& player) const;
    InspectPanelModel buildConsumable(const ConsumableViewModel& consumable) const;
    InspectPanelModel buildConsumable(const ConsumableDefinition& consumable) const;
    InspectPanelModel buildRelic(const RelicDefinition& relic) const;
    InspectPanelModel buildDroneSlot(const DroneSlotViewModel& droneSlot) const;

    InspectPanelModel buildCard(
        const CardDefinition& definition,
        const CardViewModel& card
    ) const;

private:
    void appendStatusEntry(InspectPanelModel& model, const StatusViewModel& status) const;
    void appendStatusEntry(InspectPanelModel& model, const std::string& statusId, int amount) const;
    void appendEffectEntry(InspectPanelModel& model, const std::string& titleTextId, const EffectDefinition& effect) const;

    std::string textOrFallback(const TextId& textId, const std::string& fallback) const;
    std::string rawTextOrFallback(const std::string& textId, const std::string& fallback) const;
    std::string formatRawText(const std::string& textId, const std::string& fallback, const std::vector<std::pair<std::string, std::string>>& variables) const;
    std::string keywordName(CardKeyword keyword) const;
    std::string keywordDescription(CardKeyword keyword) const;
    std::string statusName(const std::string& statusId) const;
    std::string statusDescription(const std::string& statusId) const;
    std::string statusRuleDescription(const std::string& statusId) const;
    std::string statusTypeText(StatusType type) const;
    std::string statusDurationText(StatusDurationRule rule) const;
    std::string effectSummary(const EffectDefinition& effect) const;
    std::string droneActionSummary(const DroneActionDefinition& action) const;
    std::string relicRarityText(RelicRarity rarity) const;
    std::string consumableRarityText(ConsumableRarity rarity) const;
    std::string relicModifierSummary(const RelicModifierDefinition& modifier) const;
    std::string relicTriggerSummary(const RelicTriggerDefinition& trigger) const;
    std::string targetText(EffectTarget target) const;
    std::string valueText(const EffectValue& value) const;

private:
    const ContentRegistry& content_;
    const LocalizationManager& localization_;
};
