#pragma once

#include "data/CardDatabase.hpp"
#include "effects/EffectDefinition.hpp"
#include "effects/EffectValue.hpp"
#include "localization/LocalizationManager.hpp"
#include "localization/TextFormatter.hpp"
#include "rewards/RewardOption.hpp"
#include "rewards/RewardSelection.hpp"
#include "rewards/RewardState.hpp"
#include "scenes/Scene.hpp"
#include "ui/UiFont.hpp"

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

    const RewardOption* activeOption() const;
    RewardOption* activeOption();

    std::string optionTitle(const RewardOption& option) const;
    std::string cardName(const CardId& cardId) const;
    std::string cardDescription(const CardId& cardId) const;

    static std::string effectValueText(const EffectValue& value);
    static std::string rangeToString(int minimum, int maximum);
    static void fillVariablesFromEffect(
        TextFormatter::Variables& variables,
        const EffectDefinition& effect
    );

private:
    const UiFont& font_;
    const LocalizationManager& localization_;
    const CardDatabase& cards_;
    RewardState reward_;
    RewardSelection selection_;
    std::function<void(RewardSelection)> onContinue_;

    std::optional<std::size_t> activeOptionIndex_;
    std::optional<std::size_t> selectedCardIndex_;
    bool cardChoiceOpen_ = false;
};
