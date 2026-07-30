#pragma once

#include "data/EnemyDatabase.hpp"
#include "localization/LocalizationManager.hpp"
#include "relics/RelicDatabase.hpp"
#include "run/RunCompletionType.hpp"
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
        const RelicDatabase& relics,
        const RunState& run,
        std::string nextFloorName,
        std::string runModeLabel,
        RunCompletionType completionType,
        bool canContinueToNextFloor,
        std::function<void()> onContinue,
        std::function<void()> onMainMenu
    );

    void update(float deltaSeconds) override;
    void render() const override;

private:
    Rectangle panelBounds() const;
    Rectangle continueButtonBounds() const;
    Rectangle mainMenuButtonBounds() const;
    Rectangle statusBounds() const;
    void renderRunModeBanner(Rectangle panel) const;
    bool isVictoryCompletion() const;
    bool isContentComplete() const;
    bool isFinalRunCompletion() const;
    std::string titleText() const;
    std::string statusText() const;
    std::string bossSummary() const;
    std::string floorProgressSummary() const;
    std::string hpSummary() const;
    std::string relicSummary() const;
    std::string consumableSummary() const;
    std::vector<std::pair<std::string, std::string>> statRows() const;

private:
    const UiFont& font_;
    const LocalizationManager& localization_;
    const EnemyDatabase& enemies_;
    const RelicDatabase& relics_;
    const RunState& run_;
    std::string nextFloorName_;
    std::string runModeLabel_;
    RunCompletionType completionType_ = RunCompletionType::InProgress;
    bool canContinueToNextFloor_ = false;
    std::function<void()> onContinue_;
    std::function<void()> onMainMenu_;
};
