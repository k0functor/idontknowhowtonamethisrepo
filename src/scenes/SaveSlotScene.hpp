#pragma once

#include "localization/LocalizationManager.hpp"
#include "profile/ProfileManager.hpp"
#include "scenes/Scene.hpp"
#include "ui/UiFont.hpp"

#include <cstddef>
#include <functional>

class SaveSlotScene final : public Scene {
public:
    SaveSlotScene(
        const UiFont& font,
        const LocalizationManager& localization,
        const ProfileManager& profiles,
        std::function<bool(std::size_t)> hasRunSave,
        std::function<void(std::size_t)> onNewRun,
        std::function<void(std::size_t)> onContinueRun,
        std::function<void(std::size_t)> onDeleteRun,
        std::function<void()> onBack
    );

    void update(float deltaSeconds) override;
    void render() const override;

private:
    Rectangle newRunButtonBounds(std::size_t slotIndex) const;
    Rectangle continueButtonBounds(std::size_t slotIndex) const;
    Rectangle deleteButtonBounds(std::size_t slotIndex) const;
    Rectangle slotPanelBounds(std::size_t slotIndex) const;

private:
    const UiFont& font_;
    const LocalizationManager& localization_;
    const ProfileManager& profiles_;
    std::function<bool(std::size_t)> hasRunSave_;
    std::function<void(std::size_t)> onNewRun_;
    std::function<void(std::size_t)> onContinueRun_;
    std::function<void(std::size_t)> onDeleteRun_;
    std::function<void()> onBack_;
};
