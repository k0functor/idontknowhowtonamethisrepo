#pragma once

#include "scenes/Scene.hpp"
#include "ui/UiFont.hpp"

#include <functional>

class SplashScene final : public Scene {
public:
    SplashScene(const UiFont& font, std::function<void()> onFinished);

    void update(float deltaSeconds) override;
    void render() const override;

private:
    const UiFont& font_;
    std::function<void()> onFinished_;
    float elapsedSeconds_ = 0.f;
    bool finished_ = false;
};
