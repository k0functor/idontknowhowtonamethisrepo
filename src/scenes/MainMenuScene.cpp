#include "MainMenuScene.hpp"

#include "ui/BasicUi.hpp"

#include <raylib.h>

MainMenuScene::MainMenuScene(
    const UiFont& font,
    const LocalizationManager& localization,
    std::function<void()> onPlay,
    std::function<void()> onSettings,
    std::function<void()> onExit
)
    : font_(font),
      localization_(localization),
      onPlay_(std::move(onPlay)),
      onSettings_(std::move(onSettings)),
      onExit_(std::move(onExit)) {}

void MainMenuScene::update(float) {
    const Vector2 mouse = GetMousePosition();
    const float centerX = GetScreenWidth() * 0.5f;
    const float startY = 260.f;
    const float width = 260.f;
    const float height = 54.f;
    const float gap = 18.f;

    const Rectangle play{centerX - width * 0.5f, startY, width, height};
    const Rectangle settings{centerX - width * 0.5f, startY + (height + gap), width, height};
    const Rectangle credits{centerX - width * 0.5f, startY + 2.f * (height + gap), width, height};
    const Rectangle exit{centerX - width * 0.5f, startY + 3.f * (height + gap), width, height};

    if (BasicUi::contains(play, mouse) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        onPlay_();
        return;
    }

    if (BasicUi::contains(settings, mouse) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        if (onSettings_) {
            onSettings_();
        }
        return;
    }

    if (BasicUi::contains(credits, mouse) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        notification_ = localization_.get(TextId("ui.credits_later"));
    }

    if (BasicUi::contains(exit, mouse) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        onExit_();
    }
}

void MainMenuScene::render() const {
    const Vector2 mouse = GetMousePosition();
    const float centerX = GetScreenWidth() * 0.5f;
    const float startY = 260.f;
    const float width = 260.f;
    const float height = 54.f;
    const float gap = 18.f;

    BasicUi::drawCenteredText(
        font_,
        localization_.get(TextId("game.title")),
        Rectangle{0.f, 90.f, static_cast<float>(GetScreenWidth()), 90.f},
        48.f,
        Color{240, 240, 250, 255}
    );

    BasicUi::drawButton(font_, Rectangle{centerX - width * 0.5f, startY, width, height}, localization_.get(TextId("ui.play")), mouse);
    BasicUi::drawButton(font_, Rectangle{centerX - width * 0.5f, startY + (height + gap), width, height}, localization_.get(TextId("ui.settings")), mouse);
    BasicUi::drawButton(font_, Rectangle{centerX - width * 0.5f, startY + 2.f * (height + gap), width, height}, localization_.get(TextId("ui.credits")), mouse);
    BasicUi::drawButton(font_, Rectangle{centerX - width * 0.5f, startY + 3.f * (height + gap), width, height}, localization_.get(TextId("ui.exit")), mouse);

    if (!notification_.empty()) {
        BasicUi::drawCenteredText(
            font_,
            notification_,
            Rectangle{0.f, 620.f, static_cast<float>(GetScreenWidth()), 40.f},
            18.f,
            Color{190, 195, 215, 255}
        );
    }
}
