#pragma once

#include "scenes/Scene.hpp"
#include "ui/UiFont.hpp"

#include <functional>
#include <string>

class MainMenuScene final : public Scene {
public:
    MainMenuScene(
        const UiFont& font,
        std::function<void()> onPlay,
        std::function<void()> onExit
    );

    void update(float deltaSeconds) override;
    void render() const override;

private:
    const UiFont& font_;
    std::function<void()> onPlay_;
    std::function<void()> onExit_;

    mutable std::string notification_;
};
