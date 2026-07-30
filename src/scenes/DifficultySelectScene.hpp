#pragma once

#include "run/DifficultyDefinition.hpp"
#include "scenes/Scene.hpp"
#include "ui/UiFont.hpp"
#include "localization/LocalizationManager.hpp"

#include <functional>
#include <optional>
#include <vector>

class DifficultySelectScene final : public Scene {
public:
    DifficultySelectScene(
        const UiFont& font,
        const LocalizationManager& localization,
        std::vector<const DifficultyDefinition*> difficulties,
        bool replacesExistingRunSave,
        std::function<void(DifficultyId)> onDifficultySelected,
        std::function<void()> onBack
    );

    void update(float deltaSeconds) override;
    void render() const override;

private:
    Rectangle difficultyButtonBounds(std::size_t difficultyIndex) const;
    Rectangle backButtonBounds() const;
    Rectangle replaceConfirmModalBounds() const;
    Rectangle replaceConfirmCancelButtonBounds() const;
    Rectangle replaceConfirmStartButtonBounds() const;

    void requestDifficulty(DifficultyId difficultyId);
    void confirmDifficulty();
    void cancelDifficultyConfirmation();

private:
    const UiFont& font_;
    const LocalizationManager& localization_;
    std::vector<const DifficultyDefinition*> difficulties_;
    bool replacesExistingRunSave_ = false;
    std::function<void(DifficultyId)> onDifficultySelected_;
    std::function<void()> onBack_;
    std::optional<DifficultyId> pendingDifficultyId_;
};
