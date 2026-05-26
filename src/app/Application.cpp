#include "Application.hpp"

#include <raylib.h>

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <thread>

namespace {
int toWindowDimension(const unsigned int value) {
    return static_cast<int>(std::min<unsigned int>(value, static_cast<unsigned int>(std::numeric_limits<int>::max())));
}

float clampDeltaSeconds(const float deltaSeconds) {
    constexpr float maxDeltaSeconds = 0.1f;
    return std::clamp(deltaSeconds, 0.f, maxDeltaSeconds);
}
}

Application::Application()
    : config_(AppConfig::loadFromFile("config/app.json")),
      random_(12345u) {
    initializeWindow();
    loadLocalization();
    loadContent();
    applyWindowSettings();
    createGameFlow();
}

Application::~Application() {
    gameFlow_.reset();

    if (windowInitialized_) {
        CloseWindow();
        windowInitialized_ = false;
    }
}

std::int32_t Application::run() {
    while (!WindowShouldClose()) {
        const float deltaSeconds = clampDeltaSeconds(GetFrameTime());

        processEvents();
        update(deltaSeconds);

        if (gameFlow_ != nullptr && gameFlow_->exitRequested()) {
            break;
        }

        render();

        if (!IsWindowFocused()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
    }

    return 0;
}

void Application::processEvents() {
    // raylib exposes input through polling functions.
}

void Application::update(const float deltaSeconds) {
    if (gameFlow_ != nullptr) {
        gameFlow_->update(deltaSeconds);
    }
}

void Application::render() {
    BeginDrawing();
    ClearBackground(Color{20, 20, 24, 255});

    if (gameFlow_ != nullptr) {
        gameFlow_->render();
    }

    EndDrawing();
}

void Application::initializeWindow() {
    const int width = toWindowDimension(config_.window.width);
    const int height = toWindowDimension(config_.window.height);

    SetConfigFlags(FLAG_MSAA_4X_HINT | FLAG_WINDOW_RESIZABLE);
    InitWindow(width, height, config_.window.title.c_str());
    windowInitialized_ = true;

    if (config_.window.fullscreen) {
        ToggleFullscreen();
    }
}

void Application::applyWindowSettings() {
    if (config_.window.verticalSync) {
        SetWindowState(FLAG_VSYNC_HINT);
        return;
    }

    ClearWindowState(FLAG_VSYNC_HINT);

    if (config_.window.frameRateLimit > 0) {
        SetTargetFPS(static_cast<int>(config_.window.frameRateLimit));
    } else {
        SetTargetFPS(0);
    }
}

namespace {
std::filesystem::path localizationSourceFor(
    const std::filesystem::path& localizationRoot,
    const std::string& localeCode
) {
    const std::filesystem::path directoryPath = localizationRoot / localeCode;

    if (std::filesystem::is_directory(directoryPath)) {
        return directoryPath;
    }

    return localizationRoot / (localeCode + ".json");
}
}

void Application::loadLocalization() {
    const auto localizationPath = config_.paths.data / "localization";

    localization_.loadBundle(
        Locale::russian(),
        localizationSourceFor(localizationPath, "ru")
    );

    localization_.loadBundle(
        Locale::english(),
        localizationSourceFor(localizationPath, "en")
    );

    localization_.setMissingTextPolicy(
        config_.debug.enabled
            ? MissingTextPolicy::Throw
            : MissingTextPolicy::ShowTextId
    );

    localization_.setCurrentLocale(config_.locale);
}

void Application::loadContent() {
    content_.loadFromDataDirectory(config_.paths.data);

    if (config_.debug.enabled) {
        std::cout << "Loaded cards: " << content_.cards().size() << '\n';
        std::cout << "Loaded enemies: " << content_.enemies().size() << '\n';
        std::cout << "Loaded statuses: " << content_.statuses().size() << '\n';
        std::cout << "Loaded relics: " << content_.relics().size() << '\n';
        std::cout << "Loaded actors: " << content_.actors().size() << '\n';
        std::cout << "Loaded archetypes: " << content_.archetypes().size() << '\n';
        std::cout << "Loaded difficulties: " << content_.difficulties().size() << '\n';
        std::cout << "Loaded consumables: " << content_.consumables().size() << '\n';
    }
}

void Application::createGameFlow() {
    gameFlow_ = std::make_unique<GameFlowController>(
        content_,
        localization_,
        random_,
        config_.paths.assets
    );
}
