#pragma once

#include "run/DifficultyDefinition.hpp"
#include "scenes/Scene.hpp"
#include "ui/UiFont.hpp"
#include "localization/LocalizationManager.hpp"

#include <functional>
#include <vector>

class DifficultySelectScene final : public Scene {
public:
    DifficultySelectScene(
        const UiFont& font,
        const LocalizationManager& localization,
        std::vector<const DifficultyDefinition*> difficulties,
        std::function<void(DifficultyId)> onDifficultySelected,
        std::function<void()> onBack
    );

    void update(float deltaSeconds) override;
    void render() const override;

private:
    const UiFont& font_;
    const LocalizationManager& localization_;
    std::vector<const DifficultyDefinition*> difficulties_;
    std::function<void(DifficultyId)> onDifficultySelected_;
    std::function<void()> onBack_;
};
