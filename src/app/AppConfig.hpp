#pragma once

#include "localization/Locale.hpp"

#include <cstdint>
#include <filesystem>
#include <string>

struct WindowConfig {
    std::string title = "I Dont Know How To Name This Game";

    std::uint32_t width = 1280;
    std::uint32_t height = 720;

    bool fullscreen = false;
    bool verticalSync = false;

    std::uint32_t frameRateLimit = 60;
};

struct PathConfig {
    std::filesystem::path assets = "assets";
    std::filesystem::path data = "data";
    std::filesystem::path saves = "saves";
};

struct DebugConfig {
    bool enabled = false;
};

struct AppConfig {
    WindowConfig window;
    PathConfig paths;
    DebugConfig debug;

    Locale locale = Locale::russian();

    static AppConfig loadFromFile(const std::filesystem::path& filePath);
};
