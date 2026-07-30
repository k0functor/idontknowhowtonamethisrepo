#include "EventOutcomeScene.hpp"

#include "ui/BasicUi.hpp"
#include "ui/VirtualViewport.hpp"

#include <algorithm>
#include <utility>

EventOutcomeScene::EventOutcomeScene(
    const UiFont& font,
    const LocalizationManager& localization,
    const CardDatabase& cards,
    const RelicDatabase& relics,
    const ConsumableDatabase& consumables,
    RunEventChoiceResult result,
    std::function<void()> onContinue
)
    : font_(font),
      localization_(localization),
      cards_(cards),
      relics_(relics),
      consumables_(consumables),
      result_(std::move(result)),
      onContinue_(std::move(onContinue)) {}

void EventOutcomeScene::update(float) {
    const Vector2 mouse = GetMousePosition();
    const bool continueClicked = BasicUi::contains(continueButtonBounds(), mouse) &&
        IsMouseButtonPressed(MOUSE_BUTTON_LEFT);

    if (continueClicked || IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_ESCAPE)) {
        onContinue_();
    }
}

void EventOutcomeScene::render() const {
    const Rectangle panel = panelBounds();

    BasicUi::drawCenteredText(
        font_,
        localization_.get(TextId("event.outcome.title")),
        Rectangle{0.f, 72.f, static_cast<float>(VirtualViewport::width()), 54.f},
        40.f,
        Color{244, 233, 188, 255}
    );

    DrawRectangleRounded(panel, 0.04f, 14, Color{29, 31, 41, 250});
    DrawRectangleRoundedLinesEx(panel, 0.04f, 14, 2.f, Color{111, 122, 150, 255});

    const std::vector<std::string> lines = outcomeLines();
    float y = panel.y + 48.f;
    const float maxWidth = panel.width - 96.f;
    for (const std::string& line : lines) {
        const std::vector<std::string> wrapped = BasicUi::wrapText(font_, line, 22.f, maxWidth);
        for (const std::string& wrappedLine : wrapped) {
            if (y > panel.y + panel.height - 96.f) {
                break;
            }
            BasicUi::drawText(font_, wrappedLine, Vector2{panel.x + 48.f, y}, 22.f, Color{216, 222, 238, 255});
            y += 31.f;
        }
        y += 7.f;
    }

    const Vector2 mouse = GetMousePosition();
    BasicUi::drawButton(font_, continueButtonBounds(), localization_.get(TextId("event.outcome.continue")), mouse);
}

Rectangle EventOutcomeScene::panelBounds() const {
    const float width = std::min(760.f, static_cast<float>(VirtualViewport::width()) - 100.f);
    const float height = std::min(470.f, static_cast<float>(VirtualViewport::height()) - 190.f);
    return Rectangle{
        (static_cast<float>(VirtualViewport::width()) - width) * 0.5f,
        160.f,
        width,
        height
    };
}

Rectangle EventOutcomeScene::continueButtonBounds() const {
    const Rectangle panel = panelBounds();
    return Rectangle{panel.x + panel.width - 236.f, panel.y + panel.height - 70.f, 188.f, 44.f};
}

std::vector<std::string> EventOutcomeScene::outcomeLines() const {
    if (result_.outcomes.empty()) {
        return {localization_.get(TextId("event.outcome.no_changes"))};
    }

    std::vector<std::string> lines;
    lines.reserve(result_.outcomes.size());
    for (const RunEventOutcomeEntry& entry : result_.outcomes) {
        lines.push_back(outcomeLine(entry));
    }
    return lines;
}

std::string EventOutcomeScene::outcomeLine(const RunEventOutcomeEntry& entry) const {
    switch (entry.type) {
        case RunEventOutcomeType::GoldGained:
            return localization_.format(TextId("event.outcome.gold_gained"), {{"amount", std::to_string(entry.amount)}});

        case RunEventOutcomeType::GoldLost:
            return localization_.format(TextId("event.outcome.gold_lost"), {{"amount", std::to_string(entry.amount)}});

        case RunEventOutcomeType::CardGained:
            return localization_.format(TextId("event.outcome.card_gained"), {{"name", cardName(entry.contentId)}});

        case RunEventOutcomeType::CardRemoved:
            return localization_.format(TextId("event.outcome.card_removed"), {{"name", cardName(entry.contentId)}});

        case RunEventOutcomeType::RelicGained:
            return localization_.format(TextId("event.outcome.relic_gained"), {{"name", relicName(entry.contentId)}});

        case RunEventOutcomeType::ConsumableGained:
            return localization_.format(TextId("event.outcome.consumable_gained"), {{"name", consumableName(entry.contentId)}});

        case RunEventOutcomeType::StressGained:
            return localization_.format(TextId("event.outcome.stress_gained"), {{"amount", std::to_string(entry.amount)}});

        case RunEventOutcomeType::StressLost:
            return localization_.format(TextId("event.outcome.stress_lost"), {{"amount", std::to_string(entry.amount)}});

        case RunEventOutcomeType::HpLost:
            return localization_.format(TextId("event.outcome.hp_lost"), {{"amount", std::to_string(entry.amount)}});

        case RunEventOutcomeType::HpHealed:
            return localization_.format(TextId("event.outcome.hp_healed"), {{"amount", std::to_string(entry.amount)}});

        case RunEventOutcomeType::Nothing:
            return localization_.get(TextId("event.outcome.no_changes"));
    }

    return localization_.get(TextId("event.outcome.no_changes"));
}

std::string EventOutcomeScene::cardName(const std::string& id) const {
    const CardId cardId(id);
    if (!cards_.contains(cardId)) {
        return id;
    }
    return localization_.get(cards_.get(cardId).nameTextId);
}

std::string EventOutcomeScene::relicName(const std::string& id) const {
    const RelicId relicId(id);
    if (!relics_.contains(relicId)) {
        return id;
    }
    return localization_.get(relics_.get(relicId).nameTextId);
}

std::string EventOutcomeScene::consumableName(const std::string& id) const {
    const ConsumableId consumableId(id);
    if (!consumables_.contains(consumableId)) {
        return id;
    }
    return localization_.get(consumables_.get(consumableId).nameTextId);
}
