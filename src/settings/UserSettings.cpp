#include "UserSettings.hpp"

UserSettings UserSettings::fromAppConfig(const AppConfig& config) {
    UserSettings settings;

    settings.locale = config.locale;
    settings.window.width = config.window.width;
    settings.window.height = config.window.height;
    settings.window.fullscreen = config.window.fullscreen;
    settings.window.verticalSync = config.window.verticalSync;
    settings.window.frameRateLimit = config.window.frameRateLimit;
    settings.debug.enabled = config.debug.enabled;

    return settings;
}

void UserSettings::applyTo(AppConfig& config) const {
    config.locale = locale;
    config.window.width = window.width;
    config.window.height = window.height;
    config.window.fullscreen = window.fullscreen;
    config.window.verticalSync = window.verticalSync;
    config.window.frameRateLimit = window.frameRateLimit;
    config.debug.enabled = debug.enabled;
}
