#include "Application.hpp"

#include "data/ContentValidator.hpp"
#include "settings/UserSettingsRepository.hpp"

#include <raylib.h>

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <thread>

namespace {
    constexpr unsigned int safeWindowedWidth = 1280u;
    constexpr unsigned int safeWindowedHeight = 720u;

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
      userSettings_(UserSettingsRepository::loadOrCreate(
          config_.paths.saves / "settings.json",
          UserSettings::fromAppConfig(config_)
      )),
      random_(12345u) {
    applyUserSettingsToConfig();
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
    SetExitKey(KEY_NULL);
    windowInitialized_ = true;

    if (config_.window.fullscreen) {
        enterBorderlessFullscreen();
    }
}

void Application::applyUserSettingsToConfig() {
    userSettings_.applyTo(config_);
}

void Application::applyWindowSettings() {
    SetWindowTitle(config_.window.title.c_str());

    if (config_.window.fullscreen) {
        enterBorderlessFullscreen();
    } else {
        if (borderlessFullscreenActive_ || IsWindowFullscreen()) {
            leaveBorderlessFullscreen(safeWindowedWidth, safeWindowedHeight);
            config_.window.width = safeWindowedWidth;
            config_.window.height = safeWindowedHeight;
            userSettings_.window.width = safeWindowedWidth;
            userSettings_.window.height = safeWindowedHeight;
        } else {
            const unsigned int width = config_.window.width;
            const unsigned int height = config_.window.height;
            if (GetScreenWidth() != toWindowDimension(width) || GetScreenHeight() != toWindowDimension(height)) {
                SetWindowSize(toWindowDimension(width), toWindowDimension(height));
                centerWindow(width, height);
            }
        }
    }

    if (config_.window.verticalSync) {
        SetWindowState(FLAG_VSYNC_HINT);
    } else {
        ClearWindowState(FLAG_VSYNC_HINT);
    }

    if (config_.window.frameRateLimit > 0) {
        SetTargetFPS(static_cast<int>(config_.window.frameRateLimit));
    } else {
        SetTargetFPS(0);
    }
}

void Application::enterBorderlessFullscreen() {
    if (IsWindowFullscreen()) {
        ToggleFullscreen();
    }

    const int monitor = GetCurrentMonitor();
    const Vector2 monitorPosition = GetMonitorPosition(monitor);
    const int monitorWidth = GetMonitorWidth(monitor);
    const int monitorHeight = GetMonitorHeight(monitor);

#ifdef FLAG_BORDERLESS_WINDOWED_MODE
    SetWindowState(FLAG_BORDERLESS_WINDOWED_MODE);
#endif
    SetWindowState(FLAG_WINDOW_UNDECORATED);
    SetWindowPosition(static_cast<int>(monitorPosition.x), static_cast<int>(monitorPosition.y));
    SetWindowSize(monitorWidth, monitorHeight);

    borderlessFullscreenActive_ = true;
}

void Application::leaveBorderlessFullscreen(const unsigned int width, const unsigned int height) {
    if (IsWindowFullscreen()) {
        ToggleFullscreen();
    }

#ifdef FLAG_BORDERLESS_WINDOWED_MODE
    ClearWindowState(FLAG_BORDERLESS_WINDOWED_MODE);
#endif
    ClearWindowState(FLAG_WINDOW_UNDECORATED);

    SetWindowSize(toWindowDimension(width), toWindowDimension(height));
    centerWindow(width, height);

    borderlessFullscreenActive_ = false;
}

void Application::centerWindow(const unsigned int width, const unsigned int height) {
    const int monitor = GetCurrentMonitor();
    const Vector2 monitorPosition = GetMonitorPosition(monitor);
    const int monitorWidth = GetMonitorWidth(monitor);
    const int monitorHeight = GetMonitorHeight(monitor);

    const int windowWidth = toWindowDimension(width);
    const int windowHeight = toWindowDimension(height);
    const int x = static_cast<int>(monitorPosition.x) + std::max(0, (monitorWidth - windowWidth) / 2);
    const int y = static_cast<int>(monitorPosition.y) + std::max(0, (monitorHeight - windowHeight) / 2);

    SetWindowPosition(x, y);
}

void Application::handleUserSettingsChanged(const UserSettings& settings) {
    userSettings_ = settings;
    applyUserSettingsToConfig();

    if (windowInitialized_) {
        applyWindowSettings();
    }

    UserSettingsRepository::save(config_.paths.saves / "settings.json", userSettings_);

    localization_.setMissingTextPolicy(
        config_.debug.enabled
            ? MissingTextPolicy::Throw
            : MissingTextPolicy::ShowTextId
    );

    localization_.setCurrentLocale(config_.locale);

    if (gameFlow_ != nullptr) {
        gameFlow_->notifyLocalizationChanged();
    }
}

namespace {
    std::filesystem::path requiredLocalizationDirectory(
        const std::filesystem::path& localizationRoot,
        const std::string& localeCode
    ) {
        const std::filesystem::path directoryPath = localizationRoot / localeCode;

        if (!std::filesystem::is_directory(directoryPath)) {
            throw std::runtime_error(
                "Missing localization directory '" + directoryPath.string() +
                "'. Localization is split by domain and must be stored in directories like "
                "data/localization/ru/core.json, data/localization/ru/cards.json, etc."
            );
        }

        return directoryPath;
    }
}


void Application::loadLocalization() {
    const auto localizationPath = config_.paths.data / "localization";

    localization_.loadBundle(
        Locale::russian(),
        requiredLocalizationDirectory(localizationPath, "ru")
    );

    localization_.loadBundle(
        Locale::english(),
        requiredLocalizationDirectory(localizationPath, "en")
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
    ContentValidator::validate(content_);

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
        config_.paths.assets,
        config_.paths.saves,
        userSettings_,
        [this](const UserSettings& settings) { handleUserSettingsChanged(settings); }
    );
}
