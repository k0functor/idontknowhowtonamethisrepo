#pragma once

#include "data/CardDatabase.hpp"
#include "localization/LocalizationManager.hpp"
#include "rewards/RewardSelection.hpp"
#include "rewards/RewardState.hpp"
#include "scenes/Scene.hpp"
#include "ui/UiFont.hpp"

#include <functional>
#include <optional>

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
    Rectangle cardOptionBounds(std::size_t index) const;
    Rectangle skipCardBounds() const;
    Rectangle continueButtonBounds() const;

    std::string cardName(const CardId& cardId) const;
    std::string cardDescription(const CardId& cardId) const;

private:
    const UiFont& font_;
    const LocalizationManager& localization_;
    const CardDatabase& cards_;
    RewardState reward_;
    std::function<void(RewardSelection)> onContinue_;

    std::optional<std::size_t> selectedCardIndex_;
    bool skipCardReward_ = false;
};
