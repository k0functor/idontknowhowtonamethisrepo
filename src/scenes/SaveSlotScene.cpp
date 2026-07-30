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


std::string slotSubtitle(
    const LocalizationManager& localization,
    const ProfileData& profile,
    const bool hasSave,
    const std::string& runSaveSummary
) {
    if (hasSave) {
        return runSaveSummary.empty()
            ? localization.get(TextId("save_slot.run_save_found"))
            : runSaveSummary;
    }

    if (profile.isEmpty) {
        return localization.get(TextId("save_slot.no_run_save"));
    }

    return localization.format(
        TextId("save_slot.profile_summary"),
        {
            {"rooms", std::to_string(profile.lifetimeRunStats.nodesCompleted)},
            {"enemies", std::to_string(profile.lifetimeRunStats.enemiesKilled)},
            {"bosses", std::to_string(profile.lifetimeRunStats.bossesKilled)},
            {"gold", std::to_string(profile.lifetimeRunStats.goldGained)}
        }
    );
}

}

SaveSlotScene::SaveSlotScene(
    const UiFont& font,
    const LocalizationManager& localization,
    const ProfileManager& profiles,
    std::function<bool(std::size_t)> hasRunSave,
    std::function<std::string(std::size_t)> runSaveSummary,
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
      statusMessage_(std::move(statusMessage)) {
    runSaveSummaries_.reserve(profiles_.slots().size());
    for (std::size_t slotIndex = 0; slotIndex < profiles_.slots().size(); ++slotIndex) {
        runSaveSummaries_.push_back(runSaveSummary ? runSaveSummary(slotIndex) : std::string{});
    }
}

void SaveSlotScene::update(float) {
    const Vector2 mouse = GetMousePosition();

    if (pendingDeleteSlotIndex_.has_value()) {
        if (IsKeyPressed(KEY_ESCAPE) || IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
            cancelDeleteSlot();
            return;
        }

        if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER)) {
            confirmDeleteSlot();
            return;
        }

        if (BasicUi::contains(deleteConfirmCancelButtonBounds(), mouse) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            cancelDeleteSlot();
            return;
        }

        if (BasicUi::contains(deleteConfirmDeleteButtonBounds(), mouse) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            confirmDeleteSlot();
            return;
        }

        return;
    }

    if (BasicUi::contains(backButtonBounds(), mouse) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        onBack_();
        return;
    }

    for (std::size_t i = 0; i < profiles_.slots().size(); ++i) {
        const bool hasSave = hasRunSave_ ? hasRunSave_(i) : false;
        const bool hasProfile = !profiles_.slots()[i].data.isEmpty;
        const bool canDeleteSlot = hasSave || hasProfile;

        if (BasicUi::contains(newRunButtonBounds(i), mouse) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            onNewRun_(i);
            return;
        }

        if (hasSave && BasicUi::contains(continueButtonBounds(i), mouse) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            onContinueRun_(i);
            return;
        }

        if (canDeleteSlot && BasicUi::contains(deleteButtonBounds(i), mouse) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            requestDeleteSlot(i);
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

        const std::string subtitle = slotSubtitle(
            localization_,
            profile,
            hasSave,
            i < runSaveSummaries_.size() ? runSaveSummaries_[i] : std::string{}
        );

        BasicUi::drawTextFitted(
            font_,
            subtitle,
            Vector2{panel.x + 22.f, panel.y + 54.f},
            panel.width - 396.f,
            18.f,
            13.f,
            Color{178, 184, 204, 255}
        );

        BasicUi::drawButton(font_, newRunButtonBounds(i), localization_.get(TextId("save_slot.new_run")), mouse);
        BasicUi::drawButton(font_, continueButtonBounds(i), localization_.get(TextId("save_slot.continue_run")), mouse, hasSave);
        BasicUi::drawButton(
            font_,
            deleteButtonBounds(i),
            localization_.get(TextId("save_slot.delete_run")),
            mouse,
            hasSave || !profile.isEmpty
        );
    }

    if (pendingDeleteSlotIndex_.has_value()) {
        DrawRectangle(
            0,
            0,
            VirtualViewport::width(),
            VirtualViewport::height(),
            Color{8, 9, 13, 178}
        );

        const Rectangle modal = deleteConfirmModalBounds();
        DrawRectangleRounded(modal, 0.08f, 10, Color{31, 34, 43, 252});
        DrawRectangleRoundedLinesEx(modal, 0.08f, 10, 2.f, Color{180, 110, 105, 255});

        const std::string slotNumber = std::to_string(*pendingDeleteSlotIndex_ + 1u);
        BasicUi::drawCenteredText(
            font_,
            localization_.format(TextId("save_slot.confirm_delete.title"), {{"slot", slotNumber}}),
            Rectangle{modal.x + 28.f, modal.y + 24.f, modal.width - 56.f, 34.f},
            26.f,
            Color{248, 238, 232, 255}
        );

        const std::string description = localization_.get(TextId("save_slot.confirm_delete.description"));
        const std::vector<std::string> lines = BasicUi::wrapText(font_, description, 18.f, modal.width - 64.f);
        float lineY = modal.y + 78.f;
        for (const std::string& line : lines) {
            BasicUi::drawText(font_, line, Vector2{modal.x + 32.f, lineY}, 18.f, Color{214, 206, 198, 255});
            lineY += 24.f;
        }

        BasicUi::drawText(
            font_,
            localization_.get(TextId("save_slot.confirm_delete.controls")),
            Vector2{modal.x + 32.f, modal.y + modal.height - 102.f},
            16.f,
            Color{154, 158, 176, 255}
        );

        BasicUi::drawButton(
            font_,
            deleteConfirmCancelButtonBounds(),
            localization_.get(TextId("ui.cancel")),
            mouse
        );
        BasicUi::drawButton(
            font_,
            deleteConfirmDeleteButtonBounds(),
            localization_.get(TextId("save_slot.confirm_delete.delete")),
            mouse,
            true,
            BasicUi::ButtonStyle{
                Color{82, 40, 42, 255},
                Color{116, 54, 56, 255},
                Color{35, 36, 42, 255},
                Color{212, 116, 108, 255},
                Color{255, 238, 234, 255},
                Color{120, 124, 140, 255}
            }
        );
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

Rectangle SaveSlotScene::deleteConfirmModalBounds() const {
    constexpr float modalWidth = 620.f;
    constexpr float modalHeight = 270.f;
    return Rectangle{
        static_cast<float>(VirtualViewport::width()) * 0.5f - modalWidth * 0.5f,
        static_cast<float>(VirtualViewport::height()) * 0.5f - modalHeight * 0.5f,
        modalWidth,
        modalHeight
    };
}

Rectangle SaveSlotScene::deleteConfirmCancelButtonBounds() const {
    const Rectangle modal = deleteConfirmModalBounds();
    return Rectangle{modal.x + modal.width - 326.f, modal.y + modal.height - 62.f, 140.f, buttonHeight};
}

Rectangle SaveSlotScene::deleteConfirmDeleteButtonBounds() const {
    const Rectangle modal = deleteConfirmModalBounds();
    return Rectangle{modal.x + modal.width - 172.f, modal.y + modal.height - 62.f, 140.f, buttonHeight};
}

void SaveSlotScene::requestDeleteSlot(const std::size_t slotIndex) {
    pendingDeleteSlotIndex_ = slotIndex;
}

void SaveSlotScene::confirmDeleteSlot() {
    if (!pendingDeleteSlotIndex_.has_value()) {
        return;
    }

    const std::size_t slotIndex = *pendingDeleteSlotIndex_;
    pendingDeleteSlotIndex_.reset();
    onDeleteRun_(slotIndex);
}

void SaveSlotScene::cancelDeleteSlot() {
    pendingDeleteSlotIndex_.reset();
}
