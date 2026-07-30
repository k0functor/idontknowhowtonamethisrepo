#include "EventScene.hpp"
#include "ui/VirtualViewport.hpp"

#include "ui/BasicUi.hpp"

#include <algorithm>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

EventScene::EventScene(
    const UiFont& font,
    const LocalizationManager& localization,
    const CardDatabase& cards,
    const RelicDatabase& relics,
    const ConsumableDatabase& consumables,
    const RunState& runState,
    const RunEventDefinition& event,
    std::function<void(const RunEventChoiceDefinition&)> onChoice
)
    : font_(font),
      localization_(localization),
      previewFormatter_(localization, cards, relics, consumables),
      runState_(runState),
      event_(event),
      onChoice_(std::move(onChoice)) {}

void EventScene::update(float) {
    const Vector2 mouse = GetMousePosition();

    if (!IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        return;
    }

    for (std::size_t i = 0; i < event_.choices.size(); ++i) {
        const RunEventChoiceDefinition& choice = event_.choices[i];
        if (!BasicUi::contains(choiceBounds(i), mouse)) {
            continue;
        }

        if (!choiceAvailability(choice).available) {
            return;
        }

        onChoice_(choice);
        return;
    }
}

void EventScene::render() const {
    const Vector2 mouse = GetMousePosition();
    const Rectangle panel = panelBounds();

    BasicUi::drawCenteredText(
        font_,
        localization_.get(event_.titleTextId),
        Rectangle{0.f, 64.f, static_cast<float>(VirtualViewport::width()), 54.f},
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
        const RunEventChoiceAvailability availability = choiceAvailability(choice);
        const bool hovered = availability.available && BasicUi::contains(bounds, mouse);

        const Color fill = !availability.available
            ? Color{30, 31, 39, 245}
            : (hovered ? Color{55, 60, 78, 255} : Color{40, 43, 56, 255});
        const Color border = !availability.available
            ? Color{84, 88, 105, 255}
            : (hovered ? Color{238, 196, 86, 255} : Color{110, 120, 150, 255});
        const Color titleColor = availability.available ? Color{244, 244, 250, 255} : Color{135, 139, 155, 255};
        const Color bodyColor = availability.available ? Color{190, 198, 220, 255} : Color{132, 136, 150, 255};

        DrawRectangleRounded(bounds, 0.08f, 10, fill);
        DrawRectangleRoundedLinesEx(bounds, 0.08f, 10, 2.f, border);

        BasicUi::drawText(font_, localization_.get(choice.textTextId), Vector2{bounds.x + 22.f, bounds.y + 13.f}, 23.f, titleColor);

        std::string description = choiceDescription(choice);
        if (!availability.available) {
            const std::string unavailableText = choiceUnavailableText(availability);
            if (!unavailableText.empty()) {
                description += "\n" + unavailableText;
            }
        }

        const std::vector<std::string> paragraphs = BasicUi::wrapText(font_, description, 15.f, bounds.width - 44.f);
        float lineY = bounds.y + 48.f;
        bool unavailableLine = false;
        for (const std::string& line : paragraphs) {
            if (lineY > bounds.y + bounds.height - 18.f) {
                break;
            }
            if (line.find(localization_.get(TextId("event.choice.unavailable.prefix"))) != std::string::npos) {
                unavailableLine = true;
            }
            BasicUi::drawText(
                font_,
                line,
                Vector2{bounds.x + 22.f, lineY},
                15.f,
                unavailableLine ? Color{230, 142, 122, 255} : bodyColor
            );
            lineY += 19.f;
        }
    }
}

Rectangle EventScene::panelBounds() const {
    const float width = std::min(920.f, static_cast<float>(VirtualViewport::width()) - 90.f);
    const float height = std::min(600.f, static_cast<float>(VirtualViewport::height()) - 170.f);
    return Rectangle{
        (static_cast<float>(VirtualViewport::width()) - width) * 0.5f,
        145.f,
        width,
        height
    };
}

Rectangle EventScene::choiceBounds(const std::size_t index) const {
    const Rectangle panel = panelBounds();
    const float height = 128.f;
    const float spacing = 14.f;
    const float total = static_cast<float>(event_.choices.size()) * height + static_cast<float>(event_.choices.size() - 1) * spacing;
    const float startY = panel.y + panel.height - total - 30.f;
    return Rectangle{panel.x + 40.f, startY + static_cast<float>(index) * (height + spacing), panel.width - 80.f, height};
}

RunEventChoiceAvailability EventScene::choiceAvailability(const RunEventChoiceDefinition& choice) const {
    return evaluateRunEventChoiceRequirements(choice.requirements, runState_);
}

std::string EventScene::choiceDescription(const RunEventChoiceDefinition& choice) const {
    std::string description = !choice.descriptionTextId.value.empty()
        ? localization_.get(choice.descriptionTextId)
        : localization_.get(TextId("event.choice.no_effect"));

    const std::string preview = previewFormatter_.describeChoice(choice);
    if (!preview.empty()) {
        description += "\n" + preview;
    }

    return description;
}

std::string EventScene::choiceUnavailableText(const RunEventChoiceAvailability& availability) const {
    if (availability.available || availability.reasons.empty()) {
        return {};
    }

    std::ostringstream out;
    out << localization_.get(TextId("event.choice.unavailable.prefix"));

    for (std::size_t i = 0; i < availability.reasons.size(); ++i) {
        if (i > 0) {
            out << "; ";
        } else {
            out << " ";
        }
        out << previewFormatter_.describeBlockReason(availability.reasons[i]);
    }

    return out.str();
}
