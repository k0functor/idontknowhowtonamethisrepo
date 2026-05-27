#pragma once

#include "localization/LocalizationManager.hpp"
#include "localization/TextFormatter.hpp"
#include "scenes/Scene.hpp"
#include "settings/UserSettings.hpp"
#include "ui/UiFont.hpp"

#include <functional>
#include <string>


class SettingsScene final : public Scene {
public:
    SettingsScene(
        const UiFont& font,
        const LocalizationManager& localization,
        UserSettings settings,
        std::function<void(const UserSettings&)> onSettingsChanged,
        std::function<void()> onBack
    );

    void update(float deltaSeconds) override;
    void render() const override;

private:
    enum class RowAction {
        ToggleLocale,
        CycleResolution,
        ToggleFullscreen,
        ToggleVSync,
        CycleFrameRateLimit,
        ToggleDebug,
        Back
    };

    Rectangle rowBounds(int rowIndex) const;
    void trigger(RowAction action);
    void notifyChanged();

    std::string text(const std::string& textId) const;
    std::string format(const std::string& textId, const TextFormatter::Variables& variables) const;
    std::string onOff(bool value) const;
    std::string notificationText() const;

    std::string localeLabel() const;
    std::string resolutionLabel() const;
    std::string fullscreenLabel() const;
    std::string vSyncLabel() const;
    std::string frameRateLabel() const;
    std::string debugLabel() const;

private:
    const UiFont& font_;
    const LocalizationManager& localization_;
    UserSettings settings_;
    std::function<void(const UserSettings&)> onSettingsChanged_;
    std::function<void()> onBack_;

    mutable std::string notificationTextId_;
    mutable std::string notificationValue_;
};
