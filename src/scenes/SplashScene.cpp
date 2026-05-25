#include "SplashScene.hpp"

#include "ui/BasicUi.hpp"

#include <raylib.h>

SplashScene::SplashScene(
    const UiFont& font,
    std::function<void()> onFinished
)
    : font_(font),
      onFinished_(std::move(onFinished)) {}

void SplashScene::update(const float deltaSeconds) {
    if (finished_) {
        return;
    }

    elapsedSeconds_ += deltaSeconds;

    if (elapsedSeconds_ >= 1.2f ||
        IsKeyPressed(KEY_SPACE) ||
        IsKeyPressed(KEY_ENTER) ||
        IsKeyPressed(KEY_ESCAPE) ||
        IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        finished_ = true;
        onFinished_();
    }
}

void SplashScene::render() const {
    const int width = GetScreenWidth();
    const int height = GetScreenHeight();

    BasicUi::drawCenteredText(
        font_,
        "<(.____.)>",
        Rectangle{0.f, 0.f, static_cast<float>(width), static_cast<float>(height)},
        48.f,
        Color{235, 235, 245, 255}
    );
}
