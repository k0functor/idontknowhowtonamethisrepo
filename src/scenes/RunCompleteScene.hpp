#pragma once

#include "data/ContentRegistry.hpp"
#include "localization/LocalizationManager.hpp"
#include "run/RunEndReason.hpp"
#include "run/RunState.hpp"
#include "scenes/Scene.hpp"
#include "ui/UiFont.hpp"

#include <functional>
#include <string>
#include <utility>
#include <vector>

class RunCompleteScene final : public Scene {
public:
    RunCompleteScene(
        const UiFont& font,
        const LocalizationManager& localization,
        const ContentRegistry& content,
        RunState run,
        RunEndReason reason,
        std::function<void()> onProfileHub
    );

    void update(float deltaSeconds) override;
    void render() const override;

private:
    Rectangle panelBounds() const;
    Rectangle profileHubButtonBounds() const;
    std::string titleText() const;
    std::string runModeText() const;
    std::string archetypeName() const;
    std::string difficultyName() const;
    std::string floorName() const;
    std::string relicSummary() const;
    std::string hpSummary() const;
    std::vector<std::pair<std::string, std::string>> statRows() const;

private:
    const UiFont& font_;
    const LocalizationManager& localization_;
    const ContentRegistry& content_;
    RunState run_;
    RunEndReason reason_;
    std::function<void()> onProfileHub_;
};
