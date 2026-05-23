#pragma once

#include <cstdint>
#include <filesystem>
#include <string>

struct WindowConfig {
    std::string title = "Card Roguelike";

    uint32_t width = 1280;
    uint32_t height = 720;

    bool fullscreen = false;
    bool verticalSync = false;

    uint32_t frameRateLimit = 60;
};

struct PathConfig {
    std::filesystem::path assets = "assets";
    std::filesystem::path data = "data";
    std::filesystem::path saves = "saves";
};

struct DebugConfig {
    bool enabled = true;
};

struct AppConfig {
    WindowConfig window;
    PathConfig paths;
    DebugConfig debug;

    static AppConfig loadFromFile(const std::filesystem::path& filePath);
};
