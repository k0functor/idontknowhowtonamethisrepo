#pragma once

#include "consumables/ConsumableDatabase.hpp"
#include "data/CardDatabase.hpp"
#include "localization/LocalizationManager.hpp"
#include "rewards/RewardOption.hpp"
#include "rewards/RewardSelection.hpp"
#include "rewards/RewardState.hpp"
#include "relics/RelicDatabase.hpp"
#include "scenes/Scene.hpp"
#include "ui/UiFont.hpp"
#include "ui/InspectPanelView.hpp"

#include <cstddef>
#include <functional>
#include <optional>
#include <string>

class RewardScene final : public Scene {
public:
    RewardScene(
        const UiFont& font,
        const LocalizationManager& localization,
        const CardDatabase& cards,
        const RelicDatabase& relics,
        const ConsumableDatabase& consumables,
        RewardState reward,
        std::function<void(RewardSelection)> onContinue
    );

    void update(float deltaSeconds) override;
    void render() const override;

private:
    Rectangle rewardOptionBounds(std::size_t index) const;
    Rectangle continueButtonBounds() const;

    Rectangle cardChoiceModalBounds() const;
    Rectangle cardOptionBounds(std::size_t index) const;
    Rectangle cancelButtonBounds() const;
    Rectangle confirmButtonBounds() const;

    void takeOption(std::size_t index);
    void updateCardChoice(Vector2 mouse);
    void renderCardChoice() const;
    void renderRelicInspect(const RewardOption& option, Rectangle row) const;

    const RewardOption* activeOption() const;
    RewardOption* activeOption();

    std::string optionTitle(const RewardOption& option) const;
    std::string optionDescription(const RewardOption& option) const;
    std::string cardName(const CardId& cardId) const;
    std::string cardDescription(const CardId& cardId) const;
    std::string relicName(const std::string& relicId) const;
    std::string relicDescription(const std::string& relicId) const;
    std::string consumableName(const std::string& consumableId) const;
    std::string consumableDescription(const std::string& consumableId) const;
    std::string rewardTitle() const;
    std::string rewardHint() const;

private:
    const UiFont& font_;
    const LocalizationManager& localization_;
    const CardDatabase& cards_;
    const RelicDatabase& relics_;
    const ConsumableDatabase& consumables_;
    RewardState reward_;
    RewardSelection selection_;
    std::function<void(RewardSelection)> onContinue_;
    InspectPanelView inspectPanelView_;

    std::optional<std::size_t> activeOptionIndex_;
    std::optional<std::size_t> selectedCardIndex_;
    bool cardChoiceOpen_ = false;
};
