#pragma once

#include "data/EnemyDatabase.hpp"
#include "localization/LocalizationManager.hpp"
#include "run/RunState.hpp"
#include "scenes/Scene.hpp"
#include "ui/UiFont.hpp"

#include <functional>
#include <string>
#include <vector>

class FloorCompleteScene final : public Scene {
public:
    FloorCompleteScene(
        const UiFont& font,
        const LocalizationManager& localization,
        const EnemyDatabase& enemies,
        const RunState& run,
        std::function<void()> onContinue
    );

    void update(float deltaSeconds) override;
    void render() const override;

private:
    Rectangle panelBounds() const;
    Rectangle continueButtonBounds() const;
    std::string bossSummary() const;
    std::string hpSummary() const;
    std::vector<std::pair<std::string, std::string>> statRows() const;

private:
    const UiFont& font_;
    const LocalizationManager& localization_;
    const EnemyDatabase& enemies_;
    const RunState& run_;
    std::function<void()> onContinue_;
};
