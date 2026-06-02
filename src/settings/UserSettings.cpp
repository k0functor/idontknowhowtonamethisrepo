#include "UserSettings.hpp"

bool UserSettings::debugToolsAvailable() {
#ifdef NDEBUG
    return false;
#else
    return true;
#endif
}

UserSettings UserSettings::fromAppConfig(const AppConfig& config) {
    UserSettings settings;

    settings.locale = config.locale;
    settings.window.width = config.window.width;
    settings.window.height = config.window.height;
    settings.window.fullscreen = config.window.fullscreen;
    settings.window.verticalSync = config.window.verticalSync;
    settings.window.frameRateLimit = config.window.frameRateLimit;
    settings.debug.enabled = config.debug.enabled;
    settings.enforceBuildSafety();

    return settings;
}

void UserSettings::enforceBuildSafety() {
    if (!debugToolsAvailable()) {
        debug.enabled = false;
    }
}

void UserSettings::applyTo(AppConfig& config) const {
    config.locale = locale;
    config.window.width = window.width;
    config.window.height = window.height;
    config.window.fullscreen = window.fullscreen;
    config.window.verticalSync = window.verticalSync;
    config.window.frameRateLimit = window.frameRateLimit;
    config.debug.enabled = debugToolsAvailable() && debug.enabled;
}
