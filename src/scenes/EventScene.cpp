#include "EventScene.hpp"

#include "ui/BasicUi.hpp"

#include <algorithm>
#include <string>
#include <utility>
#include <vector>

EventScene::EventScene(
    const UiFont& font,
    const LocalizationManager& localization,
    const RunEventDefinition& event,
    std::function<void(const RunEventChoiceDefinition&)> onChoice
)
    : font_(font),
      localization_(localization),
      event_(event),
      onChoice_(std::move(onChoice)) {}

void EventScene::update(float) {
    const Vector2 mouse = GetMousePosition();

    if (!IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        return;
    }

    for (std::size_t i = 0; i < event_.choices.size(); ++i) {
        if (BasicUi::contains(choiceBounds(i), mouse)) {
            onChoice_(event_.choices[i]);
            return;
        }
    }
}

void EventScene::render() const {
    const Vector2 mouse = GetMousePosition();
    const Rectangle panel = panelBounds();

    BasicUi::drawCenteredText(
        font_,
        localization_.get(event_.titleTextId),
        Rectangle{0.f, 64.f, static_cast<float>(GetScreenWidth()), 54.f},
        40.f,
        Color{244, 233, 188, 255}
    );

    DrawRectangleRounded(panel, 0.04f, 14, Color{29, 31, 41, 250});
    DrawRectangleRoundedLinesEx(panel, 0.04f, 14, 2.f, Color{111, 122, 150, 255});

    const std::vector<std::string> descriptionLines = BasicUi::wrapText(
        font_,
        localization_.get(event_.descriptionTextId),
        20.f,
        panel.width - 80.f
    );

    float y = panel.y + 34.f;
    for (const std::string& line : descriptionLines) {
        BasicUi::drawText(font_, line, Vector2{panel.x + 40.f, y}, 20.f, Color{210, 216, 232, 255});
        y += 27.f;
    }

    for (std::size_t i = 0; i < event_.choices.size(); ++i) {
        const RunEventChoiceDefinition& choice = event_.choices[i];
        const Rectangle bounds = choiceBounds(i);
        const bool hovered = BasicUi::contains(bounds, mouse);

        DrawRectangleRounded(bounds, 0.08f, 10, hovered ? Color{55, 60, 78, 255} : Color{40, 43, 56, 255});
        DrawRectangleRoundedLinesEx(bounds, 0.08f, 10, 2.f, hovered ? Color{238, 196, 86, 255} : Color{110, 120, 150, 255});

        BasicUi::drawText(font_, localization_.get(choice.textTextId), Vector2{bounds.x + 22.f, bounds.y + 13.f}, 23.f, Color{244, 244, 250, 255});
        const std::vector<std::string> lines = BasicUi::wrapText(font_, choiceDescription(choice), 15.f, bounds.width - 44.f);
        float lineY = bounds.y + 48.f;
        for (const std::string& line : lines) {
            if (lineY > bounds.y + bounds.height - 18.f) {
                break;
            }
            BasicUi::drawText(font_, line, Vector2{bounds.x + 22.f, lineY}, 15.f, Color{190, 198, 220, 255});
            lineY += 19.f;
        }
    }
}

Rectangle EventScene::panelBounds() const {
    const float width = std::min(920.f, static_cast<float>(GetScreenWidth()) - 90.f);
    const float height = std::min(600.f, static_cast<float>(GetScreenHeight()) - 170.f);
    return Rectangle{
        (static_cast<float>(GetScreenWidth()) - width) * 0.5f,
        145.f,
        width,
        height
    };
}

Rectangle EventScene::choiceBounds(const std::size_t index) const {
    const Rectangle panel = panelBounds();
    const float height = 92.f;
    const float spacing = 18.f;
    const float total = static_cast<float>(event_.choices.size()) * height + static_cast<float>(event_.choices.size() - 1) * spacing;
    const float startY = panel.y + panel.height - total - 36.f;
    return Rectangle{panel.x + 40.f, startY + static_cast<float>(index) * (height + spacing), panel.width - 80.f, height};
}

std::string EventScene::choiceDescription(const RunEventChoiceDefinition& choice) const {
    if (!choice.descriptionTextId.value.empty()) {
        return localization_.get(choice.descriptionTextId);
    }

    return localization_.get(TextId("event.choice.no_effect"));
}
