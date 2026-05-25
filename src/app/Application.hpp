#pragma once

#include "AppConfig.hpp"
#include "core/Random.hpp"
#include "data/ContentRegistry.hpp"
#include "localization/LocalizationManager.hpp"
#include "scenes/DebugCombatScene.hpp"

#include <cstdint>
#include <memory>

class Application {
public:
    Application();
    ~Application();

    Application(const Application&) = delete;
    Application& operator=(const Application&) = delete;

    std::int32_t run();

private:
    void processEvents();
    void update(float deltaSeconds);
    void render();

    void initializeWindow();
    void applyWindowSettings();
    void loadLocalization();
    void loadContent();
    void createInitialScene();

private:
    AppConfig config_;
    LocalizationManager localization_;
    ContentRegistry content_;
    Random random_;

    std::unique_ptr<DebugCombatScene> debugCombatScene_;

    bool windowInitialized_ = false;
};
