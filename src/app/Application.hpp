#pragma once

#include "AppConfig.hpp"
#include "core/Random.hpp"
#include "data/ContentRegistry.hpp"
#include "flow/GameFlowController.hpp"
#include "localization/LocalizationManager.hpp"
#include "settings/UserSettings.hpp"

#include <cstdint>
#include <memory>

#include <raylib.h>

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
    void initializeVirtualViewport();
    void shutdownVirtualViewport();
    Rectangle virtualViewportDestination() const;
    void updateVirtualViewportMouseTransform() const;
    void applyUserSettingsToConfig();
    void applyWindowSettings();
    void enterBorderlessFullscreen();
    void leaveBorderlessFullscreen(unsigned int width, unsigned int height);
    void centerWindow(unsigned int width, unsigned int height);
    void handleUserSettingsChanged(const UserSettings& settings);
    void loadLocalization();
    void loadContent();
    void createGameFlow();

private:
    AppConfig config_;
    UserSettings userSettings_;
    LocalizationManager localization_;
    ContentRegistry content_;
    Random random_;

    std::unique_ptr<GameFlowController> gameFlow_;

    RenderTexture2D virtualRenderTexture_{};

    bool windowInitialized_ = false;
    bool virtualRenderTextureInitialized_ = false;
    bool borderlessFullscreenActive_ = false;
};
