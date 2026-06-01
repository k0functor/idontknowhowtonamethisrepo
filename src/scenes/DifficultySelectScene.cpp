#include "DifficultySelectScene.hpp"
#include "ui/VirtualViewport.hpp"

#include "ui/BasicUi.hpp"

#include <raylib.h>

DifficultySelectScene::DifficultySelectScene(
    const UiFont& font,
    const LocalizationManager& localization,
    std::vector<const DifficultyDefinition*> difficulties,
    std::function<void(DifficultyId)> onDifficultySelected,
    std::function<void()> onBack
)
    : font_(font),
      localization_(localization),
      difficulties_(std::move(difficulties)),
      onDifficultySelected_(std::move(onDifficultySelected)),
      onBack_(std::move(onBack)) {}

void DifficultySelectScene::update(float) {
    const Vector2 mouse = GetMousePosition();
    const float centerX = VirtualViewport::width() * 0.5f;
    const float width = 520.f;
    const float height = 82.f;
    const float startY = 250.f;
    const float gap = 24.f;

    for (std::size_t i = 0; i < difficulties_.size(); ++i) {
        const Rectangle bounds{centerX - width * 0.5f, startY + static_cast<float>(i) * (height + gap), width, height};
        if (BasicUi::contains(bounds, mouse) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            onDifficultySelected_(difficulties_[i]->id);
            return;
        }
    }

    const Rectangle back{32.f, 32.f, 140.f, 48.f};
    if (BasicUi::contains(back, mouse) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        onBack_();
    }
}

void DifficultySelectScene::render() const {
    const Vector2 mouse = GetMousePosition();
    const float centerX = VirtualViewport::width() * 0.5f;
    const float width = 520.f;
    const float height = 82.f;
    const float startY = 250.f;
    const float gap = 24.f;

    BasicUi::drawButton(font_, Rectangle{32.f, 32.f, 140.f, 48.f}, localization_.get(TextId("ui.back")), mouse);
    BasicUi::drawCenteredText(font_, localization_.get(TextId("difficulty_select.title")), Rectangle{0.f, 110.f, static_cast<float>(VirtualViewport::width()), 70.f}, 40.f, Color{240, 240, 250, 255});

    for (std::size_t i = 0; i < difficulties_.size(); ++i) {
        const DifficultyDefinition& difficulty = *difficulties_[i];
        const Rectangle bounds{centerX - width * 0.5f, startY + static_cast<float>(i) * (height + gap), width, height};
        const bool hovered = BasicUi::drawButton(font_, bounds, localization_.get(difficulty.nameTextId), mouse);
        (void)hovered;

        BasicUi::drawText(
            font_,
            localization_.get(difficulty.descriptionTextId),
            Vector2{bounds.x + 22.f, bounds.y + 52.f},
            17.f,
            Color{185, 190, 210, 255}
        );
    }
}
