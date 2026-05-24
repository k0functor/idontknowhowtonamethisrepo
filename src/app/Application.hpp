#pragma once

#include "AppConfig.hpp"
#include "localization/LocalizationManager.hpp"

#include <SFML/Graphics/RenderWindow.hpp>

class Application {
public:
    Application();

    std::int32_t run();

private:
    void processEvents();
    void update();
    void render();

    void applyWindowSettings();
    void loadLocalization();
    
private:
    AppConfig config_;
    LocalizationManager localization_;
    sf::RenderWindow window_;
};