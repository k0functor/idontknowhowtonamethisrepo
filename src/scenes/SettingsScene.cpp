#include "SettingsScene.hpp"
#include "ui/VirtualViewport.hpp"

#include "ui/BasicUi.hpp"

#include <array>
#include <string>
#include <utility>

#include <raylib.h>

namespace {
struct ResolutionPreset {
    unsigned int width;
    unsigned int height;
};

constexpr std::array<ResolutionPreset, 5> resolutionPresets{{
    ResolutionPreset{1280u, 720u},
    ResolutionPreset{1600u, 900u},
    ResolutionPreset{1920u, 1080u},
    ResolutionPreset{2560u, 1440u},
    ResolutionPreset{3840u, 2160u}
}};

constexpr std::array<unsigned int, 5> frameRateLimits{{0u, 30u, 60u, 120u, 144u}};
constexpr unsigned int safeWindowedWidth = 1280u;
constexpr unsigned int safeWindowedHeight = 720u;

std::string localeName(const Locale& locale) {
    if (locale == Locale::russian()) {
        return "Русский";
    }

    if (locale == Locale::english()) {
        return "English";
    }

    return locale.code();
}
}

SettingsScene::SettingsScene(
    const UiFont& font,
    const LocalizationManager& localization,
    UserSettings settings,
    std::function<void(const UserSettings&)> onSettingsChanged,
    std::function<void()> onBack,
    std::function<void()> onSaveAndExit
)
    : font_(font),
      localization_(localization),
      settings_(std::move(settings)),
      onSettingsChanged_(std::move(onSettingsChanged)),
      onBack_(std::move(onBack)),
      onSaveAndExit_(std::move(onSaveAndExit)) {}

void SettingsScene::update(float) {
    const Vector2 mouse = GetMousePosition();

    for (int row = 0; row < rowCount(); ++row) {
        if (!BasicUi::contains(rowBounds(row), mouse) || !IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            continue;
        }

        switch (row) {
            case 0: trigger(RowAction::ToggleLocale); return;
            case 1: trigger(RowAction::CycleResolution); return;
            case 2: trigger(RowAction::ToggleFullscreen); return;
            case 3: trigger(RowAction::ToggleVSync); return;
            case 4: trigger(RowAction::CycleFrameRateLimit); return;
            case 5:
                trigger(onSaveAndExit_ ? RowAction::SaveAndExit : RowAction::Back);
                return;
            case 6: trigger(RowAction::Back); return;
            default: break;
        }
    }

    if (IsKeyPressed(KEY_ESCAPE)) {
        trigger(RowAction::Back);
    }
}

void SettingsScene::render() const {
    const Vector2 mouse = GetMousePosition();

    BasicUi::drawCenteredText(
        font_,
        text("settings.title"),
        Rectangle{0.f, 60.f, static_cast<float>(VirtualViewport::width()), 70.f},
        44.f,
        Color{240, 240, 250, 255}
    );

    BasicUi::drawCenteredText(
        font_,
        text("settings.saved_to_file"),
        Rectangle{0.f, 122.f, static_cast<float>(VirtualViewport::width()), 34.f},
        18.f,
        Color{180, 186, 205, 255}
    );

    BasicUi::drawButton(font_, rowBounds(0), localeLabel(), mouse);
    BasicUi::drawButton(font_, rowBounds(1), resolutionLabel(), mouse, !settings_.window.fullscreen);
    BasicUi::drawButton(font_, rowBounds(2), fullscreenLabel(), mouse);
    BasicUi::drawButton(font_, rowBounds(3), vSyncLabel(), mouse);
    BasicUi::drawButton(font_, rowBounds(4), frameRateLabel(), mouse);

    int nextRow = 5;
    if (onSaveAndExit_) {
        BasicUi::drawButton(font_, rowBounds(nextRow), text("settings.save_and_exit"), mouse);
        ++nextRow;
    }
    BasicUi::drawButton(font_, rowBounds(nextRow), text("ui.back"), mouse);

    if (!notificationTextId_.empty()) {
        BasicUi::drawCenteredText(
            font_,
            notificationText(),
            Rectangle{0.f, static_cast<float>(VirtualViewport::height()) - 74.f, static_cast<float>(VirtualViewport::width()), 34.f},
            18.f,
            Color{196, 202, 220, 255}
        );
    }
}

int SettingsScene::rowCount() const {
    return onSaveAndExit_ ? 7 : 6;
}

Rectangle SettingsScene::rowBounds(const int rowIndex) const {
    const float width = 520.f;
    const float height = onSaveAndExit_ ? 44.f : 48.f;
    const float gap = onSaveAndExit_ ? 10.f : 14.f;
    const float startY = onSaveAndExit_ ? 178.f : 190.f;
    const float x = static_cast<float>(VirtualViewport::width()) * 0.5f - width * 0.5f;

    return Rectangle{x, startY + static_cast<float>(rowIndex) * (height + gap), width, height};
}

void SettingsScene::trigger(const RowAction action) {
    switch (action) {
        case RowAction::ToggleLocale:
            settings_.locale = settings_.locale == Locale::russian()
                ? Locale::english()
                : Locale::russian();
            notificationTextId_ = "settings.notification.locale_changed";
            notificationValue_ = localeName(settings_.locale);
            notifyChanged();
            return;

        case RowAction::CycleResolution: {
            if (settings_.window.fullscreen) {
                notificationTextId_ = "settings.notification.resolution_locked";
                notificationValue_.clear();
                return;
            }

            std::size_t nextIndex = 0;
            for (std::size_t index = 0; index < resolutionPresets.size(); ++index) {
                const ResolutionPreset preset = resolutionPresets[index];
                if (preset.width == settings_.window.width && preset.height == settings_.window.height) {
                    nextIndex = (index + 1u) % resolutionPresets.size();
                    break;
                }
            }

            settings_.window.width = resolutionPresets[nextIndex].width;
            settings_.window.height = resolutionPresets[nextIndex].height;
            notificationTextId_ = "settings.notification.window_size_changed";
            notificationValue_ = std::to_string(settings_.window.width) + "x" + std::to_string(settings_.window.height);
            notifyChanged();
            return;
        }

        case RowAction::ToggleFullscreen:
            settings_.window.fullscreen = !settings_.window.fullscreen;
            if (!settings_.window.fullscreen) {
                settings_.window.width = safeWindowedWidth;
                settings_.window.height = safeWindowedHeight;
                notificationTextId_ = "settings.notification.fullscreen_off_safe_window";
                notificationValue_ = std::to_string(settings_.window.width) + "x" + std::to_string(settings_.window.height);
            } else {
                notificationTextId_ = "settings.notification.fullscreen_on";
                notificationValue_.clear();
            }
            notifyChanged();
            return;

        case RowAction::ToggleVSync:
            settings_.window.verticalSync = !settings_.window.verticalSync;
            notificationTextId_ = settings_.window.verticalSync
                ? "settings.notification.vsync_on"
                : "settings.notification.vsync_off";
            notificationValue_.clear();
            notifyChanged();
            return;

        case RowAction::CycleFrameRateLimit: {
            std::size_t nextIndex = 0;
            for (std::size_t index = 0; index < frameRateLimits.size(); ++index) {
                if (frameRateLimits[index] == settings_.window.frameRateLimit) {
                    nextIndex = (index + 1u) % frameRateLimits.size();
                    break;
                }
            }

            settings_.window.frameRateLimit = frameRateLimits[nextIndex];
            if (settings_.window.frameRateLimit == 0u) {
                notificationTextId_ = "settings.notification.fps_off";
                notificationValue_.clear();
            } else {
                notificationTextId_ = "settings.notification.fps_changed";
                notificationValue_ = std::to_string(settings_.window.frameRateLimit);
            }
            notifyChanged();
            return;
        }


        case RowAction::SaveAndExit:
            if (onSaveAndExit_) {
                onSaveAndExit_();
            }
            return;

        case RowAction::Back:
            if (onBack_) {
                onBack_();
            }
            return;
    }
}

void SettingsScene::notifyChanged() {
    if (onSettingsChanged_) {
        onSettingsChanged_(settings_);
    }
}

std::string SettingsScene::text(const std::string& textId) const {
    return localization_.get(TextId(textId));
}

std::string SettingsScene::format(const std::string& textId, const TextFormatter::Variables& variables) const {
    return localization_.format(TextId(textId), variables);
}

std::string SettingsScene::onOff(const bool value) const {
    return text(value ? "ui.on" : "ui.off");
}

std::string SettingsScene::notificationText() const {
    if (notificationTextId_.empty()) {
        return {};
    }

    if (!notificationValue_.empty()) {
        return format(notificationTextId_, {{"value", notificationValue_}});
    }

    return text(notificationTextId_);
}

std::string SettingsScene::localeLabel() const {
    return format("settings.locale", {{"value", localeName(settings_.locale)}});
}

std::string SettingsScene::resolutionLabel() const {
    return format(
        settings_.window.fullscreen ? "settings.resolution_locked" : "settings.resolution",
        {{"value", std::to_string(settings_.window.width) + "x" + std::to_string(settings_.window.height)}}
    );
}

std::string SettingsScene::fullscreenLabel() const {
    return format("settings.fullscreen", {{"value", onOff(settings_.window.fullscreen)}});
}

std::string SettingsScene::vSyncLabel() const {
    return format("settings.vsync", {{"value", onOff(settings_.window.verticalSync)}});
}

std::string SettingsScene::frameRateLabel() const {
    if (settings_.window.frameRateLimit == 0u) {
        return format("settings.fps_limit", {{"value", text("ui.off")}});
    }

    return format("settings.fps_limit", {{"value", std::to_string(settings_.window.frameRateLimit)}});
}

