#pragma once

#include "profile/ProfileManager.hpp"
#include "scenes/Scene.hpp"
#include "ui/UiFont.hpp"

#include <cstddef>
#include <functional>

class SaveSlotScene final : public Scene {
public:
    SaveSlotScene(
        const UiFont& font,
        const ProfileManager& profiles,
        std::function<void(std::size_t)> onSlotSelected,
        std::function<void()> onBack
    );

    void update(float deltaSeconds) override;
    void render() const override;

private:
    const UiFont& font_;
    const ProfileManager& profiles_;
    std::function<void(std::size_t)> onSlotSelected_;
    std::function<void()> onBack_;
};
