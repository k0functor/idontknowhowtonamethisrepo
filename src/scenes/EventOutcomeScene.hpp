#pragma once

#include "consumables/ConsumableDatabase.hpp"
#include "data/CardDatabase.hpp"
#include "events/RunEventChoiceResult.hpp"
#include "localization/LocalizationManager.hpp"
#include "relics/RelicDatabase.hpp"
#include "scenes/Scene.hpp"
#include "ui/UiFont.hpp"

#include <functional>
#include <string>
#include <vector>

#include <raylib.h>

class EventOutcomeScene final : public Scene {
public:
    EventOutcomeScene(
        const UiFont& font,
        const LocalizationManager& localization,
        const CardDatabase& cards,
        const RelicDatabase& relics,
        const ConsumableDatabase& consumables,
        RunEventChoiceResult result,
        std::function<void()> onContinue
    );

    void update(float deltaSeconds) override;
    void render() const override;

private:
    Rectangle panelBounds() const;
    Rectangle continueButtonBounds() const;
    std::vector<std::string> outcomeLines() const;
    std::string outcomeLine(const RunEventOutcomeEntry& entry) const;
    std::string cardName(const std::string& id) const;
    std::string relicName(const std::string& id) const;
    std::string consumableName(const std::string& id) const;

private:
    const UiFont& font_;
    const LocalizationManager& localization_;
    const CardDatabase& cards_;
    const RelicDatabase& relics_;
    const ConsumableDatabase& consumables_;
    RunEventChoiceResult result_;
    std::function<void()> onContinue_;
};
