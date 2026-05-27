#pragma once

#include "events/RunEventDefinition.hpp"
#include "localization/LocalizationManager.hpp"
#include "scenes/Scene.hpp"
#include "ui/UiFont.hpp"

#include <cstddef>
#include <functional>

#include <raylib.h>

class EventScene final : public Scene {
public:
    EventScene(
        const UiFont& font,
        const LocalizationManager& localization,
        const RunEventDefinition& event,
        std::function<void(const RunEventChoiceDefinition&)> onChoice
    );

    void update(float deltaSeconds) override;
    void render() const override;

private:
    Rectangle panelBounds() const;
    Rectangle choiceBounds(std::size_t index) const;
    std::string choiceDescription(const RunEventChoiceDefinition& choice) const;

private:
    const UiFont& font_;
    const LocalizationManager& localization_;
    const RunEventDefinition& event_;
    std::function<void(const RunEventChoiceDefinition&)> onChoice_;
};
