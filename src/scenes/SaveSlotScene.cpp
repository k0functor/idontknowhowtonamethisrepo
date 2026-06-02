#include "SaveSlotScene.hpp"
#include "ui/VirtualViewport.hpp"

#include "ui/BasicUi.hpp"

#include <string>
#include <utility>

#include <raylib.h>

namespace {
constexpr float panelWidth = 760.f;
constexpr float panelHeight = 112.f;
constexpr float panelGap = 20.f;
constexpr float startY = 205.f;
constexpr float buttonHeight = 38.f;

Rectangle backButtonBounds() {
    return Rectangle{32.f, 32.f, 140.f, 48.f};
}

std::string slotTitle(
    const LocalizationManager& localization,
    const ProfileData& profile,
    const std::size_t slotIndex,
    const bool hasSave
) {
    const std::string slotNumber = std::to_string(slotIndex + 1);

    if (hasSave) {
        return localization.format(TextId("save_slot.has_run"), {{"slot", slotNumber}});
    }

    if (profile.isEmpty) {
        return localization.format(TextId("save_slot.empty"), {{"slot", slotNumber}});
    }

    return localization.format(
        TextId("save_slot.record"),
        {
            {"slot", slotNumber},
            {"victories", std::to_string(profile.victories)},
            {"defeats", std::to_string(profile.defeats)}
        }
    );
}
}

SaveSlotScene::SaveSlotScene(
    const UiFont& font,
    const LocalizationManager& localization,
    const ProfileManager& profiles,
    std::function<bool(std::size_t)> hasRunSave,
    std::function<void(std::size_t)> onNewRun,
    std::function<void(std::size_t)> onContinueRun,
    std::function<void(std::size_t)> onDeleteRun,
    std::function<void()> onBack,
    std::string statusMessage
)
    : font_(font),
      localization_(localization),
      profiles_(profiles),
      hasRunSave_(std::move(hasRunSave)),
      onNewRun_(std::move(onNewRun)),
      onContinueRun_(std::move(onContinueRun)),
      onDeleteRun_(std::move(onDeleteRun)),
      onBack_(std::move(onBack)),
      statusMessage_(std::move(statusMessage)) {}

void SaveSlotScene::update(float) {
    const Vector2 mouse = GetMousePosition();

    if (BasicUi::contains(backButtonBounds(), mouse) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        onBack_();
        return;
    }

    for (std::size_t i = 0; i < profiles_.slots().size(); ++i) {
        const bool hasSave = hasRunSave_ ? hasRunSave_(i) : false;

        if (BasicUi::contains(newRunButtonBounds(i), mouse) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            onNewRun_(i);
            return;
        }

        if (hasSave && BasicUi::contains(continueButtonBounds(i), mouse) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            onContinueRun_(i);
            return;
        }

        if (hasSave && BasicUi::contains(deleteButtonBounds(i), mouse) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            onDeleteRun_(i);
            return;
        }
    }
}

void SaveSlotScene::render() const {
    const Vector2 mouse = GetMousePosition();

    BasicUi::drawButton(font_, backButtonBounds(), localization_.get(TextId("ui.back")), mouse);
    BasicUi::drawCenteredText(
        font_,
        localization_.get(TextId("save_slot.title")),
        Rectangle{0.f, 95.f, static_cast<float>(VirtualViewport::width()), 70.f},
        40.f,
        Color{240, 240, 250, 255}
    );

    if (!statusMessage_.empty()) {
        BasicUi::drawCenteredText(
            font_,
            statusMessage_,
            Rectangle{0.f, 160.f, static_cast<float>(VirtualViewport::width()), 32.f},
            18.f,
            Color{240, 176, 136, 255}
        );
    }

    for (std::size_t i = 0; i < profiles_.slots().size(); ++i) {
        const bool hasSave = hasRunSave_ ? hasRunSave_(i) : false;
        const ProfileData& profile = profiles_.slots()[i].data;
        const Rectangle panel = slotPanelBounds(i);

        DrawRectangleRounded(panel, 0.12f, 8, Color{31, 34, 43, 245});
        DrawRectangleRoundedLinesEx(panel, 0.12f, 8, 2.f, Color{105, 112, 142, 255});

        BasicUi::drawText(
            font_,
            slotTitle(localization_, profile, i, hasSave),
            Vector2{panel.x + 22.f, panel.y + 18.f},
            24.f,
            Color{238, 238, 246, 255}
        );

        const std::string subtitle = hasSave
            ? localization_.get(TextId("save_slot.run_save_found"))
            : localization_.get(TextId("save_slot.no_run_save"));

        BasicUi::drawText(
            font_,
            subtitle,
            Vector2{panel.x + 22.f, panel.y + 54.f},
            18.f,
            Color{178, 184, 204, 255}
        );

        BasicUi::drawButton(font_, newRunButtonBounds(i), localization_.get(TextId("save_slot.new_run")), mouse);
        BasicUi::drawButton(font_, continueButtonBounds(i), localization_.get(TextId("save_slot.continue_run")), mouse, hasSave);
        BasicUi::drawButton(font_, deleteButtonBounds(i), localization_.get(TextId("save_slot.delete_run")), mouse, hasSave);
    }
}

Rectangle SaveSlotScene::slotPanelBounds(const std::size_t slotIndex) const {
    return Rectangle{
        static_cast<float>(VirtualViewport::width()) * 0.5f - panelWidth * 0.5f,
        startY + static_cast<float>(slotIndex) * (panelHeight + panelGap),
        panelWidth,
        panelHeight
    };
}

Rectangle SaveSlotScene::newRunButtonBounds(const std::size_t slotIndex) const {
    const Rectangle panel = slotPanelBounds(slotIndex);
    return Rectangle{panel.x + panel.width - 342.f, panel.y + 18.f, 150.f, buttonHeight};
}

Rectangle SaveSlotScene::continueButtonBounds(const std::size_t slotIndex) const {
    const Rectangle panel = slotPanelBounds(slotIndex);
    return Rectangle{panel.x + panel.width - 178.f, panel.y + 18.f, 150.f, buttonHeight};
}

Rectangle SaveSlotScene::deleteButtonBounds(const std::size_t slotIndex) const {
    const Rectangle panel = slotPanelBounds(slotIndex);
    return Rectangle{panel.x + panel.width - 178.f, panel.y + 64.f, 150.f, buttonHeight};
}
