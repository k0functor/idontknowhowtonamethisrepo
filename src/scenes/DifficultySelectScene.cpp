#include "DifficultySelectScene.hpp"
#include "ui/VirtualViewport.hpp"

#include "ui/BasicUi.hpp"

#include <cstddef>
#include <string>
#include <utility>

#include <raylib.h>

namespace {
constexpr float difficultyButtonWidth = 520.f;
constexpr float difficultyButtonHeight = 82.f;
constexpr float difficultyStartY = 250.f;
constexpr float difficultyGap = 24.f;
constexpr float buttonHeight = 38.f;

BasicUi::ButtonStyle destructiveButtonStyle() {
    return BasicUi::ButtonStyle{
        Color{82, 40, 42, 255},
        Color{116, 54, 56, 255},
        Color{35, 36, 42, 255},
        Color{212, 116, 108, 255},
        Color{255, 238, 234, 255},
        Color{120, 124, 140, 255}
    };
}
}

DifficultySelectScene::DifficultySelectScene(
    const UiFont& font,
    const LocalizationManager& localization,
    std::vector<const DifficultyDefinition*> difficulties,
    const bool replacesExistingRunSave,
    std::function<void(DifficultyId)> onDifficultySelected,
    std::function<void()> onBack
)
    : font_(font),
      localization_(localization),
      difficulties_(std::move(difficulties)),
      replacesExistingRunSave_(replacesExistingRunSave),
      onDifficultySelected_(std::move(onDifficultySelected)),
      onBack_(std::move(onBack)) {}

void DifficultySelectScene::update(float) {
    const Vector2 mouse = GetMousePosition();

    if (pendingDifficultyId_.has_value()) {
        if (IsKeyPressed(KEY_ESCAPE) || IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
            cancelDifficultyConfirmation();
            return;
        }

        if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER)) {
            confirmDifficulty();
            return;
        }

        if (BasicUi::contains(replaceConfirmCancelButtonBounds(), mouse) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            cancelDifficultyConfirmation();
            return;
        }

        if (BasicUi::contains(replaceConfirmStartButtonBounds(), mouse) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            confirmDifficulty();
            return;
        }

        return;
    }

    for (std::size_t i = 0; i < difficulties_.size(); ++i) {
        if (BasicUi::contains(difficultyButtonBounds(i), mouse) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            requestDifficulty(difficulties_[i]->id);
            return;
        }
    }

    if (BasicUi::contains(backButtonBounds(), mouse) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        onBack_();
    }
}

void DifficultySelectScene::render() const {
    const Vector2 mouse = GetMousePosition();

    BasicUi::drawButton(font_, backButtonBounds(), localization_.get(TextId("ui.back")), mouse);
    BasicUi::drawCenteredText(
        font_,
        localization_.get(TextId("difficulty_select.title")),
        Rectangle{0.f, 110.f, static_cast<float>(VirtualViewport::width()), 70.f},
        40.f,
        Color{240, 240, 250, 255}
    );

    for (std::size_t i = 0; i < difficulties_.size(); ++i) {
        const DifficultyDefinition& difficulty = *difficulties_[i];
        const Rectangle bounds = difficultyButtonBounds(i);
        BasicUi::drawButton(font_, bounds, localization_.get(difficulty.nameTextId), mouse);

        BasicUi::drawTextFitted(
            font_,
            localization_.get(difficulty.descriptionTextId),
            Vector2{bounds.x + 22.f, bounds.y + 52.f},
            bounds.width - 44.f,
            17.f,
            13.f,
            Color{185, 190, 210, 255}
        );
    }

    if (!replacesExistingRunSave_) {
        return;
    }

    BasicUi::drawCenteredText(
        font_,
        localization_.get(TextId("difficulty_select.replace_warning")),
        Rectangle{0.f, 195.f, static_cast<float>(VirtualViewport::width()), 28.f},
        17.f,
        Color{230, 174, 130, 255}
    );

    if (!pendingDifficultyId_.has_value()) {
        return;
    }

    DrawRectangle(
        0,
        0,
        VirtualViewport::width(),
        VirtualViewport::height(),
        Color{8, 9, 13, 178}
    );

    const Rectangle modal = replaceConfirmModalBounds();
    DrawRectangleRounded(modal, 0.08f, 10, Color{31, 34, 43, 252});
    DrawRectangleRoundedLinesEx(modal, 0.08f, 10, 2.f, Color{180, 110, 105, 255});

    BasicUi::drawCenteredText(
        font_,
        localization_.get(TextId("difficulty_select.confirm_replace.title")),
        Rectangle{modal.x + 28.f, modal.y + 24.f, modal.width - 56.f, 34.f},
        26.f,
        Color{248, 238, 232, 255}
    );

    std::string difficultyName = pendingDifficultyId_->value;
    for (const DifficultyDefinition* difficulty : difficulties_) {
        if (difficulty != nullptr && difficulty->id == *pendingDifficultyId_) {
            difficultyName = localization_.get(difficulty->nameTextId);
            break;
        }
    }

    const std::string description = localization_.format(
        TextId("difficulty_select.confirm_replace.description"),
        {{"difficulty", difficultyName}}
    );
    const std::vector<std::string> lines = BasicUi::wrapText(font_, description, 18.f, modal.width - 64.f);
    float lineY = modal.y + 78.f;
    for (const std::string& line : lines) {
        BasicUi::drawText(font_, line, Vector2{modal.x + 32.f, lineY}, 18.f, Color{214, 206, 198, 255});
        lineY += 24.f;
    }

    BasicUi::drawText(
        font_,
        localization_.get(TextId("difficulty_select.confirm_replace.controls")),
        Vector2{modal.x + 32.f, modal.y + modal.height - 102.f},
        16.f,
        Color{154, 158, 176, 255}
    );

    BasicUi::drawButton(
        font_,
        replaceConfirmCancelButtonBounds(),
        localization_.get(TextId("ui.cancel")),
        mouse
    );
    BasicUi::drawButton(
        font_,
        replaceConfirmStartButtonBounds(),
        localization_.get(TextId("difficulty_select.confirm_replace.start")),
        mouse,
        true,
        destructiveButtonStyle()
    );
}

Rectangle DifficultySelectScene::difficultyButtonBounds(const std::size_t difficultyIndex) const {
    const float centerX = static_cast<float>(VirtualViewport::width()) * 0.5f;
    return Rectangle{
        centerX - difficultyButtonWidth * 0.5f,
        difficultyStartY + static_cast<float>(difficultyIndex) * (difficultyButtonHeight + difficultyGap),
        difficultyButtonWidth,
        difficultyButtonHeight
    };
}

Rectangle DifficultySelectScene::backButtonBounds() const {
    return Rectangle{32.f, 32.f, 140.f, 48.f};
}

Rectangle DifficultySelectScene::replaceConfirmModalBounds() const {
    constexpr float modalWidth = 640.f;
    constexpr float modalHeight = 270.f;
    return Rectangle{
        static_cast<float>(VirtualViewport::width()) * 0.5f - modalWidth * 0.5f,
        static_cast<float>(VirtualViewport::height()) * 0.5f - modalHeight * 0.5f,
        modalWidth,
        modalHeight
    };
}

Rectangle DifficultySelectScene::replaceConfirmCancelButtonBounds() const {
    const Rectangle modal = replaceConfirmModalBounds();
    return Rectangle{modal.x + modal.width - 326.f, modal.y + modal.height - 62.f, 140.f, buttonHeight};
}

Rectangle DifficultySelectScene::replaceConfirmStartButtonBounds() const {
    const Rectangle modal = replaceConfirmModalBounds();
    return Rectangle{modal.x + modal.width - 172.f, modal.y + modal.height - 62.f, 140.f, buttonHeight};
}

void DifficultySelectScene::requestDifficulty(DifficultyId difficultyId) {
    if (replacesExistingRunSave_) {
        pendingDifficultyId_ = std::move(difficultyId);
        return;
    }

    onDifficultySelected_(std::move(difficultyId));
}

void DifficultySelectScene::confirmDifficulty() {
    if (!pendingDifficultyId_.has_value()) {
        return;
    }

    DifficultyId difficultyId = std::move(*pendingDifficultyId_);
    pendingDifficultyId_.reset();
    onDifficultySelected_(std::move(difficultyId));
}

void DifficultySelectScene::cancelDifficultyConfirmation() {
    pendingDifficultyId_.reset();
}
