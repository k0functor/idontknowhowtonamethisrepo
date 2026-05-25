#include "MainMenuScene.hpp"

#include "ui/BasicUi.hpp"

#include <raylib.h>

MainMenuScene::MainMenuScene(
    const UiFont& font,
    std::function<void()> onPlay,
    std::function<void()> onExit
)
    : font_(font),
      onPlay_(std::move(onPlay)),
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
        notification_ = "Настройки появятся позже. Да, меню тоже требует уважения.";
    }

    if (BasicUi::contains(credits, mouse) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        notification_ = "Титры появятся позже. Пока вся вина на нас.";
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
        "Card Roguelike",
        Rectangle{0.f, 90.f, static_cast<float>(GetScreenWidth()), 90.f},
        48.f,
        Color{240, 240, 250, 255}
    );

    BasicUi::drawButton(font_, Rectangle{centerX - width * 0.5f, startY, width, height}, "Играть", mouse);
    BasicUi::drawButton(font_, Rectangle{centerX - width * 0.5f, startY + (height + gap), width, height}, "Настройки", mouse);
    BasicUi::drawButton(font_, Rectangle{centerX - width * 0.5f, startY + 2.f * (height + gap), width, height}, "Титры", mouse);
    BasicUi::drawButton(font_, Rectangle{centerX - width * 0.5f, startY + 3.f * (height + gap), width, height}, "Выход", mouse);

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
