#pragma once

#include "active_items/ActiveItemDatabase.hpp"
#include "actors/PlayerActorDatabase.hpp"
#include "consumables/ConsumableDatabase.hpp"
#include "data/CardDatabase.hpp"
#include "localization/LocalizationManager.hpp"
#include "rewards/RewardOption.hpp"
#include "rewards/RewardSelection.hpp"
#include "rewards/RewardState.hpp"
#include "relics/RelicDatabase.hpp"
#include "run/RunState.hpp"
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
        const ActiveItemDatabase& activeItems,
        const PlayerActorDatabase& actors,
        const RunState& runState,
        RewardState reward,
        std::function<bool(RewardState&, const RewardSelection&)> onReroll,
        std::function<bool(const CardId&)> onCopyCard,
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

    Rectangle relicOwnerModalBounds() const;
    Rectangle relicOwnerOptionBounds(std::size_t index) const;
    Rectangle relicOwnerCancelButtonBounds() const;

    Rectangle activeItemModalBounds() const;
    Rectangle activeItemKeepButtonBounds() const;
    Rectangle activeItemEquipButtonBounds() const;

    void moveRelicOwnerSelection(int delta);

    void updateActiveItemChoice(Vector2 mouse);
    void renderActiveItemChoice() const;
    void openActiveItemChoice(std::size_t index);
    void closeActiveItemChoice();
    void confirmActiveItemChoice();

    void takeOption(std::size_t index);
    void updateRelicOwnerChoice(Vector2 mouse);
    void renderRelicOwnerChoice() const;
    void openRelicOwnerChoice(std::size_t index);
    void closeRelicOwnerChoice();
    void confirmRelicOwnerChoice(std::size_t actorIndex);
    bool hasMultipleRelicOwners() const;
    std::string defaultRelicOwnerId() const;
    std::string actorName(const std::string& actorDefinitionId) const;
    std::string actorHealthSummary(const RunActorState& actor) const;
    std::string actorRelicSummary(const RunActorState& actor) const;
    void updateCardChoice(Vector2 mouse);
    void renderCardChoice() const;
    void renderRelicInspect(const RewardOption& option, Rectangle row) const;
    bool copyCardHintVisible() const;

    const RewardOption* activeOption() const;
    RewardOption* activeOption();

    std::string optionTitle(const RewardOption& option) const;
    std::string cardName(const CardId& cardId) const;
    std::string cardDescription(const CardId& cardId) const;
    std::string relicName(const std::string& relicId) const;
    std::string relicDescription(const std::string& relicId) const;
    std::string consumableName(const std::string& consumableId) const;
    std::string consumableDescription(const std::string& consumableId) const;
    std::string activeItemName(const std::string& activeItemId) const;
    std::string rewardTitle() const;

private:
    const UiFont& font_;
    const LocalizationManager& localization_;
    const CardDatabase& cards_;
    const RelicDatabase& relics_;
    const ConsumableDatabase& consumables_;
    const ActiveItemDatabase& activeItems_;
    const PlayerActorDatabase& actors_;
    const RunState& runState_;
    RewardState reward_;
    RewardSelection selection_;
    std::function<bool(RewardState&, const RewardSelection&)> onReroll_;
    std::function<bool(const CardId&)> onCopyCard_;
    std::function<void(RewardSelection)> onContinue_;
    InspectPanelView inspectPanelView_;

    std::optional<std::size_t> activeOptionIndex_;
    std::optional<std::size_t> selectedCardIndex_;
    std::optional<std::size_t> activeRelicOwnerOptionIndex_;
    std::optional<std::size_t> selectedRelicOwnerIndex_;
    std::optional<std::size_t> activeItemOptionIndex_;
    bool cardChoiceOpen_ = false;
    bool relicOwnerChoiceOpen_ = false;
    bool activeItemChoiceOpen_ = false;
};
