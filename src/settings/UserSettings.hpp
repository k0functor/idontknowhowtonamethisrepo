#pragma once

#include "app/AppConfig.hpp"
#include "localization/Locale.hpp"

#include <cstdint>

struct UserWindowSettings {
    std::uint32_t width = 1280;
    std::uint32_t height = 720;
    bool fullscreen = false;
    bool verticalSync = false;
    std::uint32_t frameRateLimit = 60;
};

struct UserAudioSettings {
    float masterVolume = 1.f;
    float musicVolume = 0.8f;
    float sfxVolume = 0.8f;
};

struct UserDebugSettings {
    bool enabled = true;
};

struct UserSettings {
    Locale locale = Locale::russian();
    UserWindowSettings window;
    UserAudioSettings audio;
    UserDebugSettings debug;

    static UserSettings fromAppConfig(const AppConfig& config);

    void applyTo(AppConfig& config) const;
};
