#pragma once

#include "consumables/ConsumableDatabase.hpp"
#include "data/CardDatabase.hpp"
#include "events/RunEventDefinition.hpp"
#include "events/RunEventChoicePreviewFormatter.hpp"
#include "events/RunEventRequirement.hpp"
#include "localization/LocalizationManager.hpp"
#include "relics/RelicDatabase.hpp"
#include "run/RunState.hpp"
#include "scenes/Scene.hpp"
#include "ui/UiFont.hpp"

#include <cstddef>
#include <functional>
#include <string>

#include <raylib.h>

class EventScene final : public Scene {
public:
    EventScene(
        const UiFont& font,
        const LocalizationManager& localization,
        const CardDatabase& cards,
        const RelicDatabase& relics,
        const ConsumableDatabase& consumables,
        const RunState& runState,
        const RunEventDefinition& event,
        std::function<void(const RunEventChoiceDefinition&)> onChoice
    );

    void update(float deltaSeconds) override;
    void render() const override;

private:
    Rectangle panelBounds() const;
    Rectangle choiceBounds(std::size_t index) const;
    RunEventChoiceAvailability choiceAvailability(const RunEventChoiceDefinition& choice) const;
    std::string choiceDescription(const RunEventChoiceDefinition& choice) const;
    std::string choiceUnavailableText(const RunEventChoiceAvailability& availability) const;

private:
    const UiFont& font_;
    const LocalizationManager& localization_;
    RunEventChoicePreviewFormatter previewFormatter_;
    const RunState& runState_;
    const RunEventDefinition& event_;
    std::function<void(const RunEventChoiceDefinition&)> onChoice_;
};
